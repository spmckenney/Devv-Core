/*
 * ValidatorController.cpp
 *
 *  Created on: Mar 23, 2018
 *      Author: Nick Williams
 */

#include "ValidatorController.h"

#include <condition_variable>
#include <mutex>
#include <thread>
#include <string>
#include <ctime>
#include <boost/filesystem.hpp>

#include "DevcashWorker.h"
#include "common/devcash_exceptions.h"
#include "common/logger.h"
#include "io/message_service.h"
#include "consensus/KeyRing.h"
#include "primitives/Tier1Transaction.h"
#include "primitives/Tier2Transaction.h"

typedef std::chrono::milliseconds millisecs;

namespace fs = boost::filesystem;

namespace Devcash {

#define DEBUG_TRANSACTION_INDEX \
  (processed + (context_.get_current_node()+1)*11000000)

ValidatorController::ValidatorController(
    io::TransactionServer& server,
    io::TransactionClient& peer_client,
    size_t validator_count,
    size_t consensus_count,
    size_t batch_size,
    const KeyRing& keys,
    DevcashContext& context,
    const ChainState& prior,
    eAppMode mode,
    const std::string& stop_file)
  : server_(server)
  , peer_client_(peer_client)
  , validator_count_(validator_count)
  , consensus_count_(consensus_count)
  , keys_(keys)
  , context_(context)
  , final_chain_("final_chain_")
  , utx_pool_(prior, mode, batch_size)
  , mode_(mode)
  , stop_file_(stop_file)
  , workers_(new DevcashControllerWorker(this, validator_count_, consensus_count_, consensus_count_))
{}

ValidatorController::~ValidatorController() {
  if (workers_) {
    delete workers_;
  }
}

void ValidatorController::ValidatorCallback(DevcashMessageUniquePtr ptr) {
  std::lock_guard<std::mutex> guard(mutex_);
  if (shutdown_) return;
  if (ptr == nullptr) {
    LOG_DEBUG << "ValidatorCallback(): ptr == nullptr, ignoring";
    return;
  }
  LogDevcashMessageSummary(*ptr, "ValidatorCallback");
  try {
    LOG_DEBUG << "ValidatorController::ValidatorCallback()";
    MTR_SCOPE_FUNC();
    if (ptr->message_type == TRANSACTION_ANNOUNCEMENT) {
      DevcashMessage msg(*ptr.get());
      utx_pool_.AddTransactions(msg.data, keys_);
      if (context_.get_current_node()%context_.get_peer_count() == 0) {
        size_t block_height = final_chain_.size();
        if (block_height == 0) {
          LOG_INFO << "(spm): CreateNextProposal, utx_pool.HasProposal(): " << utx_pool_.HasProposal();
          if (!utx_pool_.HasProposal()) {
            server_.queueMessage(std::move(
                message_callbacks_.transaction_announcement_cb(keys_, final_chain_, utx_pool_, context_)));
          }
        }
      }
    } else if (ptr->message_type == GET_BLOCKS_SINCE
        || ptr->message_type == BLOCKS_SINCE || ptr->message_type == REQUEST_BLOCK) {
      LOG_DEBUG << "Unexpected message @ validator, to shard comms.\n";
      pushShardComms(std::move(ptr));
    } else {
      LOG_DEBUG << "Unexpected message @ validator, to consensus.\n";
      pushConsensus(std::move(ptr));
    }
  } catch (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "ValidatorController.ValidatorCallback()");
    StopAll();
  }
}

void ValidatorController::ConsensusCallback(DevcashMessageUniquePtr ptr) {
  std::lock_guard<std::mutex> guard(mutex_);
  MTR_SCOPE_FUNC();
  try {
    if (shutdown_) {
      LOG_DEBUG << "ValidatorController()::ConsensusCallback(): shutdown_ == true";
      return;
    }
    switch(ptr->message_type) {
      case eMessageType::FINAL_BLOCK:
        LOG_DEBUG << "ValidatorController()::ConsensusCallback(): FINAL_BLOCK";
        message_callbacks_.final_block_cb(std::move(ptr),
                                          context_,
                                          keys_,
                                          final_chain_,
                                          utx_pool_,
                                          [this](DevcashMessageUniquePtr p) {
                                            this->server_.queueMessage(std::move(p));
                                          });
        break;

      case eMessageType::PROPOSAL_BLOCK:
        LOG_DEBUG << "ValidatorController()::ConsensusCallback(): PROPOSAL_BLOCK";
        utx_pool_.get_transaction_creation_manager().set_keys(&keys_);
        message_callbacks_.proposal_block_cb(std::move(ptr),
                                             context_,
                                             keys_,
                                             final_chain_,
                                             utx_pool_.get_transaction_creation_manager(),
                                             [this](DevcashMessageUniquePtr p) { this->server_.queueMessage(std::move(p)); });
        break;

      case eMessageType::TRANSACTION_ANNOUNCEMENT:
        LOG_DEBUG << "ValidatorController()::ConsensusCallback(): TRANSACTION_ANNOUNCEMENT";
        LOG_WARNING << "Unexpected message @ consensus, to validator";
        pushValidator(std::move(ptr));
        break;

      case eMessageType::VALID:
        LOG_DEBUG << "ValidatorController()::ConsensusCallback(): VALIDATION";
        message_callbacks_.validation_block_cb(std::move(ptr),
                              context_,
                              final_chain_,
                              utx_pool_,
                              [this](DevcashMessageUniquePtr p) { this->server_.queueMessage(std::move(p)); });
        break;

      case eMessageType::REQUEST_BLOCK:
        LOG_DEBUG << "Unexpected message @ consensus, to shard comms.";
        pushShardComms(std::move(ptr));
        break;

      case eMessageType::GET_BLOCKS_SINCE:
        LOG_DEBUG << "Unexpected message @ consensus, to shard comms.\n";
        pushShardComms(std::move(ptr));
        break;

      case eMessageType::BLOCKS_SINCE:
        LOG_DEBUG << "Unexpected message @ consensus, to shard comms.\n";
        pushShardComms(std::move(ptr));
        break;

      default:
        throw DevcashMessageError("ConsensusCallback(): Unexpected message type:"
                                  + std::to_string(ptr->message_type));
        break;
    }
  } catch (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "ValidatorController.ConsensusCallback()");
    StopAll();
    throw e;
  }
}

void ValidatorController::ShardCommsCallback(DevcashMessageUniquePtr ptr) {
  std::lock_guard<std::mutex> guard(mutex_);
  MTR_SCOPE_FUNC();
  try {
    if (shutdown_) {
      LOG_INFO << "ValidatorController()::ShardCommsCallback(): shutdown_ == true";
      return;
    }
    switch(ptr->message_type) {
    case eMessageType::FINAL_BLOCK:
      LOG_WARNING << "Unexpected message @ shard comms, to consensus.\n";
            pushConsensus(std::move(ptr));
      break;

    case eMessageType::PROPOSAL_BLOCK:
      LOG_WARNING << "Unexpected message @ shard comms, to consensus.\n";
            pushConsensus(std::move(ptr));
      break;

    case eMessageType::TRANSACTION_ANNOUNCEMENT:
      LOG_WARNING << "Unexpected message @ shard comms, to validator";
            pushValidator(std::move(ptr));
      break;

    case eMessageType::VALID:
      LOG_WARNING << "Unexpected message @ shard comms, to consensus.\n";
            pushConsensus(std::move(ptr));
      break;

    case eMessageType::REQUEST_BLOCK:
      LOG_DEBUG << "ValidatorController()::ShardCommsCallback(): REQUEST_BLOCK";
      //request updates from remote shards if this chain has grown
      if (remote_blocks_ < final_chain_.size()) {
        std::vector<byte> request;
        Uint64ToBin(remote_blocks_, request);
        uint64_t node = context_.get_current_node();
        Uint64ToBin(node, request);
        if (mode_ == eAppMode::T1) {
          //request blocks from all live remote shards with the node index matching this node's index
          //in the case of benchmarking, there should be two live T2 shards,
          //so assume they are shards 1 and 2 (where T1 has the 0 index)
          std::string uri1 = context_.get_uri_from_index(context_.get_peer_count()+context_.get_current_node()%context_.get_peer_count());
          auto blocks_msg1 = std::make_unique<DevcashMessage>(uri1
                                                      , GET_BLOCKS_SINCE
                                                      , request
                                                      , remote_blocks_);
            server_.queueMessage(std::move(blocks_msg1));
          std::string uri2 = context_.get_uri_from_index(context_.get_peer_count()*2+context_.get_current_node()%context_.get_peer_count());
          auto blocks_msg2 = std::make_unique<DevcashMessage>(uri2
                                                      , GET_BLOCKS_SINCE
                                                      , request
                                                      , remote_blocks_);
            server_.queueMessage(std::move(blocks_msg2));
        } else if (mode_ == eAppMode::T2) {
          //a T2 should request new blocks from the T1 node with the same node index as itself
          std::string uri = context_.get_uri_from_index(context_.get_current_node()%context_.get_peer_count());
          auto blocks_msg = std::make_unique<DevcashMessage>(uri
                                                      , GET_BLOCKS_SINCE
                                                      , request
                                                      , remote_blocks_);
            server_.queueMessage(std::move(blocks_msg));
        }
      }
      break;

    case eMessageType::GET_BLOCKS_SINCE:
      LOG_DEBUG << "ValidatorController()::ShardCommsCallback(): GET_BLOCKS_SINCE";
      //provide blocks since requested height
      message_callbacks_.blocks_since_request_cb(std::move(ptr),
                               final_chain_,
                               context_,
                               keys_,
                               [this](DevcashMessageUniquePtr p) { this->server_.queueMessage(std::move(p));});
      break;

  case eMessageType::BLOCKS_SINCE:
      LOG_DEBUG << "ValidatorController()::ShardCommsCallback(): BLOCKS_SINCE";
      message_callbacks_.blocks_since_cb(std::move(ptr),
                        final_chain_,
                        context_,
                        keys_,
                        utx_pool_,
                        remote_blocks_);
      break;

    default:
      LOG_ERROR << "ValidatorController()::ShardCommsCallback(): Unexpected message, ignore.\n";
      break;
    }
  } catch (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "ValidatorController.ShardCommsCallback()");
    throw e;
    StopAll();
  }
}

void ValidatorController::Start() {
  MTR_SCOPE_FUNC();
  try {

    auto lambda_callback = [this](DevcashMessageUniquePtr ptr) {
      if (ptr->message_type == TRANSACTION_ANNOUNCEMENT) {
        pushValidator(std::move(ptr));
      } else if (ptr->message_type == GET_BLOCKS_SINCE
          || ptr->message_type == BLOCKS_SINCE || ptr->message_type == REQUEST_BLOCK) {
        pushShardComms(std::move(ptr));
      } else {
        pushConsensus(std::move(ptr));
      }
    };

    peer_client_.attachCallback(lambda_callback);
    peer_client_.listenTo(context_.get_shard_uri());
    peer_client_.listenTo(context_.get_uri());

    server_.startServer();
    peer_client_.startClient();

    std::vector<std::vector<byte>> transactions;
    size_t processed = 0;

    workers_->Start();

    if (context_.get_sync_host().size() > 0) {
      io::synchronize(context_.get_sync_host(), context_.get_current_node());
    } else {
      LOG_NOTICE << "ValidatorController::Start(): Starting devcash - entering forever loop in 10 sec";
      sleep(1);
    }

    // Loop for long runs
    auto ms = kMAIN_WAIT_INTERVAL;
    while (!shutdown_) {
      LOG_DEBUG << "Sleeping for " << ms
                << ": processed/batches/pending/chain_size/txs ("
                << processed << "/"
                << transactions.size() << "/"
                << utx_pool_.NumPendingTransactions() << "/"
                << final_chain_.size() << "/"
                << final_chain_.getNumTransactions() << ")";
      std::this_thread::sleep_for(millisecs(ms));

      /* Should we announce a transaction? */
      if (processed < transactions.size()) {
        auto announce_msg = std::make_unique<DevcashMessage>(context_.get_uri()
                                                             , TRANSACTION_ANNOUNCEMENT
                                                             , transactions.at(processed)
                                                             , DEBUG_TRANSACTION_INDEX);
          server_.queueMessage(std::move(announce_msg));
        processed++;
      }

      //fetch updates from other shards
      if (remote_blocks_ < final_chain_.size()) {
        std::vector<byte> request;
        auto request_msg = std::make_unique<DevcashMessage>(context_.get_uri()
		                                                    , REQUEST_BLOCK
		                                                    , request
		                                                    , remote_blocks_);
          server_.queueMessage(std::move(request_msg));
        remote_blocks_ = final_chain_.size();
      }

      /* Should we shutdown? */
      if (fs::exists(stop_file_)) {
        LOG_INFO << "Shutdown file detected. Stopping DevCash...";
        StopAll();
      }
    }

  } catch (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "ValidatorController.Start()");
    StopAll();
  }
}

void ValidatorController::StopAll() {
  CASH_TRY {
    LOG_DEBUG << "ValidatorController::StopAll()";
    shutdown_ = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    peer_client_.stopClient();
    server_.stopServer();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    workers_->StopAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "ValidatorController.StopAll()");
  }
}

void ValidatorController::pushValidator(DevcashMessageUniquePtr ptr) {
  CASH_TRY {
    LOG_DEBUG << "ValidatorController::pushValidator()";
    workers_->pushValidator(std::move(ptr));
  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "ValidatorController.pushValidator()");
  }
}

void ValidatorController::pushConsensus(DevcashMessageUniquePtr ptr) {
  CASH_TRY {
    LOG_DEBUG << "ValidatorController::pushConsensus()";
    workers_->pushConsensus(std::move(ptr));
  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "ValidatorController.pushConsensus()");
  }
}

void ValidatorController::pushShardComms(DevcashMessageUniquePtr ptr) {
  CASH_TRY {
    LOG_DEBUG << "ValidatorController::pushShardComms()";
    workers_->pushShardComms(std::move(ptr));
  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "ValidatorController.pushShardComms()");
  }
}

} //end namespace Devcash
