/*
 * DevcashController.cpp
 *
 *  Created on: Mar 23, 2018
 *      Author: Nick Williams
 */

#include "DevcashController.h"

#include <condition_variable>
#include <mutex>
#include <thread>
#include <string>
#include <ctime>
#include <boost/filesystem.hpp>

#include "DevcashWorker.h"
#include "io/message_service.h"
#include "consensus/KeyRing.h"
#include "primitives/Tier1Transaction.h"
#include "primitives/Tier2Transaction.h"

typedef std::chrono::milliseconds millisecs;

namespace fs = boost::filesystem;

namespace Devcash {

#define DEBUG_TRANSACTION_INDEX \
  (processed + (context_.get_current_node()+1)*11000000)

DevcashController::DevcashController(
    io::TransactionServer& server,
    io::TransactionClient& peer_client,
    io::TransactionClient& loopback_client,
    size_t validator_count,
    size_t consensus_count,
    size_t batch_size,
    const KeyRing& keys,
    DevcashContext& context,
    const ChainState& prior,
    eAppMode mode,
    const std::string& working_dir,
    const std::string& stop_file)
  : server_(server)
  , peer_client_(peer_client)
  , loopback_client_(loopback_client)
  , validator_count_(validator_count)
  , consensus_count_(consensus_count)
  , keys_(keys)
  , context_(context)
  , final_chain_("final_chain_")
  , utx_pool_(prior, mode, batch_size)
  , mode_(mode)
  , working_dir_(working_dir)
  , stop_file_(stop_file)
  , workers_(new DevcashControllerWorker(this, validator_count_, consensus_count_, consensus_count_))
{}

DevcashController::~DevcashController() {
  if (workers_) {
    delete workers_;
  }
}

void DevcashController::ValidatorCallback(DevcashMessageUniquePtr ptr) {
  std::lock_guard<std::mutex> guard(mutex_);
  if (shutdown_) return;
  if (ptr == nullptr) {
    LOG_DEBUG << "ValidatorCallback(): ptr == nullptr, ignoring";
    return;
  }
  LogDevcashMessageSummary(*ptr, "ValidatorCallback");
  try {
    LOG_DEBUG << "DevcashController::ValidatorCallback()";
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
    LOG_FATAL << FormatException(&e, "DevcashController.ValidatorCallback()");
    StopAll();
  }
}

void DevcashController::ConsensusCallback(DevcashMessageUniquePtr ptr) {
  std::lock_guard<std::mutex> guard(mutex_);
  MTR_SCOPE_FUNC();
  try {
    if (shutdown_) {
      LOG_DEBUG << "DevcashController()::ConsensusCallback(): shutdown_ == true";
      return;
    }
    switch(ptr->message_type) {
      case eMessageType::FINAL_BLOCK:
        LOG_DEBUG << "DevcashController()::ConsensusCallback(): FINAL_BLOCK";
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
        LOG_DEBUG << "DevcashController()::ConsensusCallback(): PROPOSAL_BLOCK";
        utx_pool_.get_transaction_creation_manager().set_keys(&keys_);
        message_callbacks_.proposal_block_cb(std::move(ptr),
                                             context_,
                                             keys_,
                                             final_chain_,
                                             utx_pool_.get_transaction_creation_manager(),
                                             [this](DevcashMessageUniquePtr p) { this->server_.queueMessage(std::move(p)); });
        waiting_ = 0;
        break;

      case eMessageType::TRANSACTION_ANNOUNCEMENT:
        LOG_DEBUG << "DevcashController()::ConsensusCallback(): TRANSACTION_ANNOUNCEMENT";
        LOG_WARNING << "Unexpected message @ consensus, to validator";
        pushValidator(std::move(ptr));
        break;

      case eMessageType::VALID:
        LOG_DEBUG << "DevcashController()::ConsensusCallback(): VALIDATION";
        message_callbacks_.validation_block_cb(std::move(ptr),
                              context_,
                              final_chain_,
                              utx_pool_,
                              working_dir_,
                              [this](DevcashMessageUniquePtr p) { this->server_.queueMessage(std::move(p)); });
        break;

      case eMessageType::REQUEST_BLOCK:
        LOG_DEBUG << "Unexpected message @ consensus, to shard comms.";
        pushShardComms(std::move(ptr));
        break;

      case eMessageType::GET_BLOCKS_SINCE:LOG_DEBUG << "Unexpected message @ consensus, to shard comms.\n";
        pushShardComms(std::move(ptr));
        break;

      case eMessageType::BLOCKS_SINCE:LOG_DEBUG << "Unexpected message @ consensus, to shard comms.\n";
        pushShardComms(std::move(ptr));
        break;

      default:LOG_ERROR << "DevcashController()::ConsensusCallback(): Unexpected message, ignore.\n";
        break;
    }
  } catch (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashController.ConsensusCallback()");
    StopAll();
    throw e;
  }
}

void DevcashController::ShardCommsCallback(DevcashMessageUniquePtr ptr) {
  std::lock_guard<std::mutex> guard(mutex_);
  MTR_SCOPE_FUNC();
  try {
    if (shutdown_) {
      LOG_INFO << "DevcashController()::ShardCommsCallback(): shutdown_ == true";
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
      LOG_DEBUG << "DevcashController()::ShardCommsCallback(): REQUEST_BLOCK";
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
      LOG_DEBUG << "DevcashController()::ShardCommsCallback(): GET_BLOCKS_SINCE";
      //provide blocks since requested height
      message_callbacks_.blocks_since_request_cb(std::move(ptr),
                               final_chain_,
                               context_,
                               keys_,
                               [this](DevcashMessageUniquePtr p) { this->server_.queueMessage(std::move(p));});
      break;

  case eMessageType::BLOCKS_SINCE:
      LOG_DEBUG << "DevcashController()::ShardCommsCallback(): BLOCKS_SINCE";
      message_callbacks_.blocks_since_cb(std::move(ptr),
                        final_chain_,
                        context_,
                        keys_,
                        utx_pool_,
                        remote_blocks_);
      break;

    default:
      LOG_ERROR << "DevcashController()::ShardCommsCallback(): Unexpected message, ignore.\n";
      break;
    }
  } catch (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashController.ShardCommsCallback()");
    throw e;
    StopAll();
  }
}

void DevcashController::ValidatorToyCallback(DevcashMessageUniquePtr ptr) {
  MTR_SCOPE_FUNC();
  LOG_DEBUG << "DevcashController::ValidatorToyCallback()";
  assert(ptr);
  if (validator_flipper_) {
    pushConsensus(std::move(ptr));
  }
  validator_flipper_ = !validator_flipper_;
}

void DevcashController::ConsensusToyCallback(DevcashMessageUniquePtr ptr) {
  MTR_SCOPE_FUNC();
  LOG_DEBUG << "DevcashController()::ConsensusToyCallback()";
  assert(ptr);
  if (consensus_flipper_) {
      server_.queueMessage(std::move(ptr));
  }
  consensus_flipper_ = !consensus_flipper_;
}

std::vector<std::vector<byte>> DevcashController::loadTransactions() {
  LOG_DEBUG << "Loading transactions from " << working_dir_;
  MTR_SCOPE_FUNC();
  std::vector<std::vector<byte>> out;

  ChainState priori;

  fs::path p(working_dir_);

  if (!is_directory(p)) {
    LOG_ERROR << "Error opening dir: " << working_dir_ << " is not a directory";
    return out;
  }

  std::mutex critical;

  std::vector<std::string> files;

  input_blocks_ = 0;
  for(auto& entry : boost::make_iterator_range(fs::directory_iterator(p), {})) {
    files.push_back(entry.path().string());
  }

  ThreadPool::ParallelFor(0, (int)files.size(), [&] (int i) {
      LOG_INFO << "Reading " << files.at(i);
      std::ifstream file(files.at(i), std::ios::binary);
      file.unsetf(std::ios::skipws);
      std::streampos file_size;
      file.seekg(0, std::ios::end);
      file_size = file.tellg();
      file.seekg(0, std::ios::beg);

      std::vector<byte> raw;
      raw.reserve(file_size);
      raw.insert(raw.begin(), std::istream_iterator<byte>(file)
                 , std::istream_iterator<byte>());
      std::vector<byte> batch;
      assert(file_size > 0);
      InputBuffer buffer(raw);
      while (buffer.getOffset() < static_cast<size_t>(file_size)) {
        //constructor increments offset by reference
        FinalBlock one_block(buffer, priori);
        Summary sum = Summary::Copy(one_block.getSummary());
        Validation val(one_block.getValidation());
        std::pair<Address, Signature> pair(val.getFirstValidation());
        int index = keys_.getNodeIndex(pair.first);
        assert(index >= 0);
        Tier1Transaction tx(sum, pair.second, (uint64_t) index, keys_);
        std::vector<byte> tx_canon(tx.getCanonical());
        batch.insert(batch.end(), tx_canon.begin(), tx_canon.end());
        input_blocks_++;
      }
      std::lock_guard<std::mutex> lock(critical);

      out.push_back(batch);
    }, 3);

  LOG_INFO << "Loaded " << std::to_string(input_blocks_) << " transactions in " << out.size() << " batches.";
  return out;
}

void DevcashController::Start() {
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

    loopback_client_.attachCallback(lambda_callback);
    loopback_client_.listenTo(context_.get_uri());

    server_.startServer();
    peer_client_.startClient();
    loopback_client_.startClient();

    std::vector<std::vector<byte>> transactions;
    size_t processed = 0;

    //setup directory for FinalBlocks
    fs::path p(working_dir_);
    if (is_directory(p)) {
      std::string shard_path(working_dir_+"/"+context_.get_shard_uri());
      fs::path shard_dir(shard_path);
      if (!is_directory(shard_dir)) fs::create_directory(shard_dir);
      if (mode_ == eAppMode::T1 && !working_dir_.empty()) {
        transactions = loadTransactions();
      }
    } else {
      LOG_ERROR << "Error opening dir: " << working_dir_ << " is not a directory";
    }
    fs::path stop_file(working_dir_+"/"+stop_file_);

    workers_->Start();

    if (context_.get_sync_host().size() > 0) {
      io::synchronize(context_.get_sync_host(), context_.get_current_node());
    } else {
      LOG_NOTICE << "DevcashController::Start(): Starting devcash - entering forever loop in 10 sec";
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
      if (fs::exists(stop_file)) {
        LOG_INFO << "Shutdown file created. Stopping DevCash...";
        StopAll();
      }
    }

  } catch (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashController.Start()");
    StopAll();
  }
}

void DevcashController::StopAll() {
  CASH_TRY {
    LOG_DEBUG << "DevcashController::StopAll()";
    shutdown_ = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    peer_client_.stopClient();
    loopback_client_.stopClient();
    server_.stopServer();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    workers_->StopAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashController.StopAll()");
  }
}

void DevcashController::pushValidator(DevcashMessageUniquePtr ptr) {
  CASH_TRY {
    LOG_DEBUG << "DevcashController::pushValidator()";
    workers_->pushValidator(std::move(ptr));
  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashController.pushValidator()");
  }
}

void DevcashController::pushConsensus(DevcashMessageUniquePtr ptr) {
  CASH_TRY {
    LOG_DEBUG << "DevcashController::pushConsensus()";
    workers_->pushConsensus(std::move(ptr));
  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashController.pushConsensus()");
  }
}

void DevcashController::pushShardComms(DevcashMessageUniquePtr ptr) {
  CASH_TRY {
    LOG_DEBUG << "DevcashController::pushShardComms()";
    workers_->pushShardComms(std::move(ptr));
  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashController.pushShardComms()");
  }
}

} //end namespace Devcash
