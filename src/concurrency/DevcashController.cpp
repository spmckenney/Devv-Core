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
    const int validatorCount,
    const int consensusCount,
    const int generateCount,
    const int batchSize,
    const size_t transaction_limit,
    const KeyRing& keys,
    DevcashContext& context,
    const ChainState& prior,
    eAppMode mode,
    std::string scan_dir)
  : server_(server)
  , peer_client_(peer_client)
  , loopback_client_(loopback_client)
  , validator_count_(validatorCount)
  , consensus_count_(consensusCount)
  , generate_count_(generateCount)
  , batch_size_(batchSize)
  , transaction_limit_(transaction_limit)
  , keys_(keys)
  , context_(context)
  , final_chain_("final_chain_")
  , utx_pool_(prior, mode, batchSize)
  , mode_(mode)
  , scan_dir_(scan_dir)
  , workers_(new DevcashControllerWorker(this, validator_count_, consensus_count_, consensus_count_))
{}

DevcashController::~DevcashController() {
  if (workers_) {
    delete workers_;
  }
}

void DevcashController::validatorCallback(DevcashMessageUniquePtr ptr) {
  std::lock_guard<std::mutex> guard(mutex_);
  if (shutdown_) return;
  if (ptr == nullptr) {
    LOG_DEBUG << "validatorCallback(): ptr == nullptr, ignoring";
    return;
  }
  LogDevcashMessageSummary(*ptr, "validatorCallback");
  CASH_TRY {
    LOG_DEBUG << "DevcashController::validatorCallback()";
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
  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashController.validatorCallback()");
    stopAll();
  }
}

void DevcashController::consensusCallback(DevcashMessageUniquePtr ptr) {
  std::lock_guard<std::mutex> guard(mutex_);
  MTR_SCOPE_FUNC();
  CASH_TRY {
    if (shutdown_) {
      LOG_DEBUG << "DevcashController()::consensusCallback(): shutdown_ == true";
      return;
    }
    switch(ptr->message_type) {
      case eMessageType::FINAL_BLOCK:
        LOG_DEBUG << "DevcashController()::consensusCallback(): FINAL_BLOCK";
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
        LOG_DEBUG << "DevcashController()::consensusCallback(): PROPOSAL_BLOCK";
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
        LOG_DEBUG << "DevcashController()::consensusCallback(): TRANSACTION_ANNOUNCEMENT";
        LOG_WARNING << "Unexpected message @ consensus, to validator";
        pushValidator(std::move(ptr));
        break;

      case eMessageType::VALID:
        LOG_DEBUG << "DevcashController()::consensusCallback(): VALIDATION";
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

      case eMessageType::GET_BLOCKS_SINCE:LOG_DEBUG << "Unexpected message @ consensus, to shard comms.\n";
        pushShardComms(std::move(ptr));
        break;

      case eMessageType::BLOCKS_SINCE:LOG_DEBUG << "Unexpected message @ consensus, to shard comms.\n";
        pushShardComms(std::move(ptr));
        break;

      default:LOG_ERROR << "DevcashController()::consensusCallback(): Unexpected message, ignore.\n";
        break;
    }
  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashController.consensusCallback()");
    CASH_THROW(e);
    stopAll();
  }
}

void DevcashController::shardCommsCallback(DevcashMessageUniquePtr ptr) {
  std::lock_guard<std::mutex> guard(mutex_);
  MTR_SCOPE_FUNC();
  //CASH_TRY {
    if (shutdown_) {
      LOG_DEBUG << "DevcashController()::shardCommsCallback(): shutdown_ == true";
      return;
    }
    switch(ptr->message_type) {
    case eMessageType::FINAL_BLOCK:
      LOG_DEBUG << "Unexpected message @ shard comms, to consensus.\n";
            pushConsensus(std::move(ptr));
      break;

    case eMessageType::PROPOSAL_BLOCK:
      LOG_DEBUG << "Unexpected message @ shard comms, to consensus.\n";
            pushConsensus(std::move(ptr));
      break;

    case eMessageType::TRANSACTION_ANNOUNCEMENT:
      LOG_WARNING << "Unexpected message @ shard comms, to validator";
            pushValidator(std::move(ptr));
      break;

    case eMessageType::VALID:
      LOG_DEBUG << "Unexpected message @ shard comms, to consensus.\n";
            pushConsensus(std::move(ptr));
      break;

    case eMessageType::REQUEST_BLOCK:
      LOG_DEBUG << "DevcashController()::shardCommsCallback(): REQUEST_BLOCK";
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
      LOG_DEBUG << "DevcashController()::shardCommsCallback(): GET_BLOCKS_SINCE";
      //provide blocks since requested height
      message_callbacks_.blocks_since_request_cb(std::move(ptr),
                               final_chain_,
                               context_,
                               keys_,
                               [this](DevcashMessageUniquePtr p) { this->server_.queueMessage(std::move(p));});
      break;

  case eMessageType::BLOCKS_SINCE:
      LOG_DEBUG << "DevcashController()::shardCommsCallback(): BLOCKS_SINCE";
      message_callbacks_.blocks_since_cb(std::move(ptr),
                        final_chain_,
                        context_,
                        keys_,
                        utx_pool_,
                        remote_blocks_);
      break;

    default:
      LOG_ERROR << "DevcashController()::shardCommsCallback(): Unexpected message, ignore.\n";
      break;
    }
    /*
  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashController.shardCommsCallback()");
    CASH_THROW(e);
    stopAll();
  }
    */
}

void DevcashController::validatorToyCallback(DevcashMessageUniquePtr ptr) {
  MTR_SCOPE_FUNC();
  LOG_DEBUG << "DevcashController::validatorToyCallback()";
  assert(ptr);
  if (validator_flipper_) {
    pushConsensus(std::move(ptr));
  }
  validator_flipper_ = !validator_flipper_;
}

void DevcashController::consensusToyCallback(DevcashMessageUniquePtr ptr) {
  MTR_SCOPE_FUNC();
  LOG_DEBUG << "DevcashController()::consensusToyCallback()";
  assert(ptr);
  if (consensus_flipper_) {
      server_.queueMessage(std::move(ptr));
  }
  consensus_flipper_ = !consensus_flipper_;
}

std::vector<std::vector<byte>> DevcashController::generateTransactions() {
  MTR_SCOPE_FUNC();
  std::vector<std::vector<byte>> out;
  EVP_MD_CTX* ctx;
  if(!(ctx = EVP_MD_CTX_create())) {
    LOG_FATAL << "Could not create signature context!";
    CASH_THROW("Could not create signature context!");
  }

  Address inn_addr = keys_.getInnAddr();

  size_t addr_count = keys_.CountWallets();

  size_t counter = 0;
  size_t batch_counter = 0;
  while (counter < generate_count_) {
    std::vector<byte> batch;
    while (batch_counter < batch_size_) {
      std::vector<Transfer> xfers;
      Transfer inn_transfer(inn_addr, 0, -1*addr_count, 0);
      xfers.push_back(inn_transfer);
      for (size_t i=0; i<addr_count; ++i) {
        Transfer transfer(keys_.getWalletAddr(i), 0, 1, 0);
        xfers.push_back(transfer);
      }
      Tier2Transaction inn_tx(eOpType::Create, xfers
                         , GetMillisecondsSinceEpoch()+(1000000*(context_.get_current_node()+1)*(batch_counter+1))
          , keys_.getKey(inn_addr), keys_);
      std::vector<byte> inn_canon(inn_tx.getCanonical());
      batch.insert(batch.end(), inn_canon.begin(), inn_canon.end());
      LOG_DEBUG << "generateTransactions(): generated inn_tx with sig: " << ToHex(inn_tx.getSignature());
      batch_counter++;
      for (size_t i=0; i<addr_count; ++i) {
        for (size_t j=0; j<addr_count; ++j) {
          if (i==j) continue;
          std::vector<Transfer> peer_xfers;
          Transfer sender(keys_.getWalletAddr(i), 0, -1, 0);
          peer_xfers.push_back(sender);
          Transfer receiver(keys_.getWalletAddr(j), 0, 1, 0);
          peer_xfers.push_back(receiver);
          Tier2Transaction peer_tx(eOpType::Exchange, peer_xfers
                              , GetMillisecondsSinceEpoch()+(1000000*(context_.get_current_node()+1)*(i+1)*(j+1))
                              , keys_.getWalletKey(i), keys_);
          std::vector<byte> peer_canon(peer_tx.getCanonical());
          batch.insert(batch.end(), peer_canon.begin(), peer_canon.end());
          LOG_TRACE << "generateTransactions(): generated tx with sig: " << ToHex(peer_tx.getSignature());
          batch_counter++;
          if (batch_counter >= batch_size_) break;
        } //end inner for
        if (batch_counter >= batch_size_) break;
      } //end outer for
      if (batch_counter >= batch_size_) break;
    } //end batch while
    out.push_back(batch);
    counter += batch_counter;
    batch.clear();
    batch_counter = 0;
  } //end counter while

  LOG_INFO << "Generated " << counter << " transactions in " << out.size() << " batches.";
  return out;
}

std::vector<std::vector<byte>> DevcashController::loadTransactions() {
  LOG_DEBUG << "Loading transactions from " << scan_dir_;
  MTR_SCOPE_FUNC();
  std::vector<std::vector<byte>> out;

  ChainState priori;

  fs::path p(scan_dir_);

  if (!is_directory(p)) {
    LOG_ERROR << "Error opening dir: " << scan_dir_ << " is not a directory";
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
      size_t offset = 0;
      std::vector<byte> batch;
      assert(file_size > 0);
      while (offset < static_cast<size_t>(file_size)) {
        //constructor increments offset by reference
        FinalBlock one_block(raw, priori, offset);
        Summary sum(one_block.getSummary());
        Validation val(one_block.getValidation());
        std::pair<Address, Signature> pair(val.getFirstValidation());
        int index = keys_.getNodeIndex(pair.first);
        assert(index > 0);
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

std::vector<byte> DevcashController::Start() {
  MTR_SCOPE_FUNC();
  std::vector<byte> out;
  CASH_TRY {

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

    workers_->Start();

    if (mode_ == eAppMode::T2 && generate_count_ > 0) {
      std::lock_guard<std::mutex> guard(mutex_);
      LOG_INFO << "Generate Transactions.";
      transactions = generateTransactions();
      LOG_INFO << "Finished Generating " << transactions.size() * batch_size_ << " Transactions.";
    } else if (mode_ == eAppMode::T1 && !scan_dir_.empty()) {
      transactions = loadTransactions();
    } else {
      LOG_WARNING << "Not loading or generating: " << scan_dir_;
    }

    if (context_.get_sync_host().size() > 0) {
      io::synchronize(context_.get_sync_host(), context_.get_current_node());
    } else {
      LOG_NOTICE << "DevcashController::Start(): Starting devcash - entering forever loop in 10 sec";
      sleep(1);
    }

    // Loop for long runs
    auto ms = kMAIN_WAIT_INTERVAL;
    while (true) {
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
      } else if (!utx_pool_.HasPendingTransactions()) {
        if (final_chain_.getNumTransactions() >= transaction_limit_
            && transaction_limit_ > 0) {
          LOG_INFO << "Reached transaction limit shutting down.";
          stopAll();
        } else {
          LOG_INFO << "Waiting for peers to finish...";
        }
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

      if (shutdown_) break;
    }

    //write final chain to output
    std::vector<byte> final_chain_bin(final_chain_.BinaryDump());
    out.insert(out.end(), final_chain_bin.begin()
            , final_chain_bin.end());

  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashController.Start()");
    stopAll();
  }
  return out;
}

void DevcashController::stopAll() {
  CASH_TRY {
    LOG_DEBUG << "DevcashController::stopAll()";
    shutdown_ = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    peer_client_.stopClient();
    loopback_client_.stopClient();
    server_.stopServer();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    workers_->StopAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashController.stopAll()");
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
