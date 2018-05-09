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
#include <time.h>
#include <boost/filesystem.hpp>

#include "DevcashWorker.h"
#include "io/message_service.h"
#include "consensus/KeyRing.h"
#include "primitives/Tier1Transaction.h"
#include "primitives/Tier2Transaction.h"

typedef std::chrono::milliseconds millisecs;

namespace fs = boost::filesystem;

namespace Devcash {

#define DEBUG_PROPOSAL_INDEX \
  ((block_height+1) + (context.get_current_node()+1)*1000000)

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
  , keys_(keys)
  , context_(context)
  , final_chain_("final_chain_")
  , utx_pool_(prior, mode)
  , mode_(mode)
  , scan_dir_(scan_dir)
  , workers_(new DevcashControllerWorker(this, validator_count_, consensus_count_))
{}

DevcashController::~DevcashController() {
  if (workers_) {
    delete workers_;
  }
}

DevcashMessageUniquePtr CreateNextProposal(const KeyRing& keys,
                        Blockchain& final_chain,
                        UnrecordedTransactionPool& utx_pool,
                        const DevcashContext& context) {
  MTR_SCOPE_FUNC();
  size_t block_height = final_chain.size();

  if (!(block_height % 100) || !((block_height + 1) % 100)) {
    LOG_NOTICE << "Processing @ final_chain_.size: (" << std::to_string(block_height) << ")";
  }

  if (!utx_pool.HasProposal() && utx_pool.HasPendingTransactions()) {
      Hash prev_hash = final_chain.getHighestMerkleRoot();
      ChainState prior = final_chain.getHighestChainState();
      utx_pool.ProposeBlock(prev_hash, prior, keys, context);
  }

  LOG_INFO << "Proposal #"+std::to_string(block_height+1)+".";

  std::vector<byte> proposal(utx_pool.getProposal());

  // Create message
  auto propose_msg = std::make_unique<DevcashMessage>("peers"
                                                      , PROPOSAL_BLOCK
                                                      , proposal
                                                      , DEBUG_PROPOSAL_INDEX);
  // FIXME (spm): define index value somewhere
  LogDevcashMessageSummary(*propose_msg, "CreateNextProposal");
  return propose_msg;

}

void DevcashController::ValidatorCallback(DevcashMessageUniquePtr ptr) {
  std::lock_guard<std::mutex> guard(mutex_);
  if (shutdown_) return;
  if (ptr == nullptr) {
    LOG_DEBUG << "ValidatorCallback(): ptr == nullptr, ignoring";
    return;
  }
  LogDevcashMessageSummary(*ptr, "ValidatorCallback");
  CASH_TRY {
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
            server_.QueueMessage(std::move(
              CreateNextProposal(keys_,final_chain_,utx_pool_,context_)));
          }
        }
      }
    } else {
      LOG_DEBUG << "Unexpected message @ validator, to consensus.\n";
      PushConsensus(std::move(ptr));
    }
  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashController.ValidatorCallback()");
    StopAll();
  }
}

bool HandleFinalBlock(DevcashMessageUniquePtr ptr,
                      const DevcashContext& context,
                      const KeyRing& keys,
                      Blockchain& final_chain,
                      UnrecordedTransactionPool& utx_pool,
                      std::function<void(DevcashMessageUniquePtr)> callback) {
  MTR_SCOPE_FUNC();
  //Make the incoming block final
  //if pending proposal, makes sure it is still valid
  //if no pending proposal, check if should make one
  DevcashMessage msg(*ptr.get());

  LogDevcashMessageSummary(*ptr, "HandleFinalBlock()");

  ChainState prior = final_chain.getHighestChainState();
  FinalPtr top_block = std::make_shared<FinalBlock>(utx_pool.FinalizeRemoteBlock(
                                               msg.data, prior, keys));
  final_chain.push_back(top_block);
  LOG_NOTICE << "final_chain.push_back(): Estimated rate: (ntxs/duration): rate -> "
             << "(" << final_chain.getNumTransactions() << "/"
             << utx_pool.getElapsedTime() << "): "
             << final_chain.getNumTransactions() / (utx_pool.getElapsedTime()/1000) << " txs/sec";

  // Did we send a message
  bool sent_message = false;

  if (utx_pool.HasProposal()) {
    ChainState current = top_block->getChainState();
    Hash prev_hash = top_block->getMerkleRoot();
    utx_pool.ReverifyProposal(prev_hash, current, keys);
  }

  size_t block_height = final_chain.size();

  if (!utx_pool.HasPendingTransactions()) {
    LOG_INFO << "All pending transactions processed.";
  } else if ((block_height+1)%context.get_peer_count() == context.get_current_node()%context.get_peer_count()) {
    if (!utx_pool.HasProposal()) {
      callback(std::move(CreateNextProposal(keys,final_chain,utx_pool,context)));
      sent_message = true;
    }
  }
  return sent_message;
}

bool HandleProposalBlock(DevcashMessageUniquePtr ptr,
                         const DevcashContext& context,
                         const KeyRing& keys,
                         Blockchain& final_chain,
                         TransactionCreationManager& tcm,
                         std::function<void(DevcashMessageUniquePtr)> callback) {
  MTR_SCOPE_FUNC();
  //validate block
  //if valid, push VALID message
  DevcashMessage msg(*ptr.get());

  LogDevcashMessageSummary(*ptr, "HandleProposalBlock() -> Incoming");

  ChainState prior = final_chain.getHighestChainState();
  ProposedBlock to_validate(msg.data, prior, keys, tcm);
  if (!to_validate.validate(keys)) {
    LOG_WARNING << "ProposedBlock is invalid!";
    return false;
  }
  if (!to_validate.SignBlock(keys, context)) {
    LOG_WARNING << "ProposedBlock.SignBlock failed!";
    return false;
  }
  LOG_DEBUG << "Proposed block is valid.";
  std::vector<byte> validation(to_validate.getValidationData());

  auto valid = std::make_unique<DevcashMessage>("peers",
                                                VALID,
                                                validation,
                                                ptr->index);
  LogDevcashMessageSummary(*valid, "HandleProposalBlock() -> Validation");
  callback(std::move(valid));
  return true;
}

bool HandleValidationBlock(DevcashMessageUniquePtr ptr,
                           const DevcashContext& context,
                           Blockchain& final_chain,
                           UnrecordedTransactionPool& utx_pool,
                           std::function<void(DevcashMessageUniquePtr)> callback) {
  MTR_SCOPE_FUNC();
  bool sent_message = false;
  DevcashMessage msg(*ptr.get());

  LogDevcashMessageSummary(*ptr, "HandleValidationBlock() -> Incoming");

  if (utx_pool.CheckValidation(msg.data, context)) {
    //block can be finalized, so finalize
    LOG_DEBUG << "Ready to finalize block.";
    FinalPtr top_block = std::make_shared<FinalBlock>(utx_pool.FinalizeLocalBlock());
    final_chain.push_back(top_block);
    LOG_NOTICE << "final_chain.push_back(): Estimated rate: (ntxs/duration): rate -> "
               << "(" << final_chain.getNumTransactions() << "/"
               << utx_pool.getElapsedTime() << "): "
               << final_chain.getNumTransactions() / (utx_pool.getElapsedTime()/1000) << " txs/sec";

    std::vector<byte> final_msg = top_block->getCanonical();

    auto final_block = std::make_unique<DevcashMessage>("peers", FINAL_BLOCK, final_msg, ptr->index);
    LogDevcashMessageSummary(*final_block, "HandleValidationBlock() -> Final block");
    callback(std::move(final_block));
    sent_message = true;
  }

  return sent_message;
}

bool HandleBlocksSinceRequest(DevcashMessageUniquePtr ptr,
                              Blockchain& final_chain,
                              const DevcashContext& context,
                              const KeyRing& keys,
                              std::function<void(DevcashMessageUniquePtr)> callback) {
  LogDevcashMessageSummary(*ptr, "HandleBlocksSinceRequest() -> Incoming");
  if (ptr.get()->data.size() < 16) {
    LOG_WARNING << "BlockSinceRequest is too small!";
    return false;
  }
  uint64_t height = BinToUint32(ptr.get()->data, 0);
  uint64_t node = BinToUint32(ptr.get()->data, 8);
  std::vector<byte> raw = final_chain.PartialBinaryDump(height);

  if (context.get_app_mode() == eAppMode::T2) {
    size_t offset = 0;
    std::vector<byte> tier1_data;
    ChainState temp;
    while (offset < raw.size()) {
      //constructor increments offset by reference
      FinalBlock one_block(raw, temp, offset);
      Summary sum(one_block.getSummary());
      Validation val(one_block.getValidation());
      std::pair<Address, Signature> pair(val.getFirstValidation());
      int index = keys.getNodeIndex(pair.first);
      Tier1Transaction tx(sum, pair.second, (uint64_t) index, keys);
      std::vector<byte> tx_canon(tx.getCanonical());
      tier1_data.insert(tier1_data.end(), tx_canon.begin(), tx_canon.end());
    }
    auto response = std::make_unique<DevcashMessage>(
      context.get_uri_from_index(node), TRANSACTION_ANNOUNCEMENT
      , tier1_data, ptr->index);
    callback(std::move(response));
    return true;
  } else if (context.get_app_mode() == eAppMode::T1) {
    uint64_t covered_height = final_chain.size()-1;
    std::vector<byte> bin_height;
    Uint64ToBin(covered_height, bin_height);
    //put height at beginning of message
    raw.insert(raw.begin(), bin_height.begin(), bin_height.end());
    auto response = std::make_unique<DevcashMessage>(context.get_uri_from_index(node)
      , BLOCKS_SINCE
      , raw, ptr->index);
    callback(std::move(response));
    return true;
  }
  return false;
}

bool HandleBlocksSince(DevcashMessageUniquePtr ptr,
                              Blockchain& final_chain,
                              DevcashContext context,
                              const KeyRing& keys,
                              const UnrecordedTransactionPool& utx_pool,
                              uint64_t& remote_blocks) {
  LogDevcashMessageSummary(*ptr, "HandleBlocksSince() -> Incoming");

  if (ptr.get()->data.size() < 8) {
      LOG_WARNING << "BlockSince is too small!";
      return false;
    }
    uint64_t height = BinToUint64(ptr.get()->data, 0);

  if (context.get_app_mode() == eAppMode::T2) {
    size_t offset = 8;
    std::vector<Address> wallets = keys.getDesignatedWallets(context.get_current_shard());
    ChainState state = final_chain.getHighestChainState();
    while (offset < ptr.get()->data.size()) {
      //constructor increments offset by reference
      FinalBlock one_block(ptr.get()->data, state, offset);
      uint64_t elapsed = getEpoch() - one_block.getBlockTime();
      Summary sum(one_block.getSummary());
      for (auto const& addr : wallets) {
        std::vector<SmartCoin> coins = sum.getCoinsByAddr(addr, elapsed);
        for (auto const& coin : coins) {
          state.addCoin(coin);
        }
      }
      //TODO: update upcoming state in utx pool
    }
    if (height > remote_blocks) remote_blocks = height;
    LOG_INFO << "Finished updating local state for Tier1 block height: "+std::to_string(height);
  }
  return false;
}

void DevcashController::ConsensusCallback(DevcashMessageUniquePtr ptr) {
  std::lock_guard<std::mutex> guard(mutex_);
  MTR_SCOPE_FUNC();
  CASH_TRY {
    if (shutdown_) {
      LOG_DEBUG << "DevcashController()::ConsensusCallback(): shutdown_ == true";
      return;
    }
    switch(ptr->message_type) {
    case eMessageType::FINAL_BLOCK:
      LOG_DEBUG << "DevcashController()::ConsensusCallback(): FINAL_BLOCK";
      HandleFinalBlock(std::move(ptr),
                       context_,
                       keys_,
                       final_chain_,
                       utx_pool_,
                       [this](DevcashMessageUniquePtr p) { this->server_.QueueMessage(std::move(p));});
      break;

    case eMessageType::PROPOSAL_BLOCK:
      LOG_DEBUG << "DevcashController()::ConsensusCallback(): PROPOSAL_BLOCK";
      utx_pool_.get_transaction_creation_manager().set_keys(&keys_);
      HandleProposalBlock(std::move(ptr),
                          context_,
                          keys_,
                          final_chain_,
                          utx_pool_.get_transaction_creation_manager(),
                          [this](DevcashMessageUniquePtr p) { this->server_.QueueMessage(std::move(p));});
      waiting_ = 0;
      break;

    case eMessageType::TRANSACTION_ANNOUNCEMENT:
      LOG_DEBUG << "DevcashController()::ConsensusCallback(): TRANSACTION_ANNOUNCEMENT";
      LOG_WARNING << "Unexpected message @ consensus, to validator";
      PushValidator(std::move(ptr));
      break;

    case eMessageType::VALID:
      LOG_DEBUG << "DevcashController()::ConsensusCallback(): VALIDATION";
      HandleValidationBlock(std::move(ptr),
                            context_,
                            final_chain_,
                            utx_pool_,
                            [this](DevcashMessageUniquePtr p) { this->server_.QueueMessage(std::move(p));});
      break;

    case eMessageType::REQUEST_BLOCK:
      LOG_DEBUG << "DevcashController()::ConsensusCallback(): REQUEST_BLOCK";
      //deprecate this message?
      break;

    case eMessageType::GET_BLOCKS_SINCE:
      LOG_DEBUG << "DevcashController()::ConsensusCallback(): GET_BLOCKS_SINCE";
      //provide blocks since requested height
      HandleBlocksSinceRequest(std::move(ptr),
                               final_chain_,
                               context_,
                               keys_,
                               [this](DevcashMessageUniquePtr p) { this->server_.QueueMessage(std::move(p));});
      break;

  case eMessageType::BLOCKS_SINCE:
      LOG_DEBUG << "DevcashController()::ConsensusCallback(): BLOCKS_SINCE";
      HandleBlocksSince(std::move(ptr),
                        final_chain_,
                        context_,
                        keys_,
                        utx_pool_,
                        remote_blocks_);
      break;

    default:
      LOG_ERROR << "DevcashController()::ConsensusCallback(): Unexpected message, ignore.\n";
      break;
    }
  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashController.ConsensusCallback()");
    CASH_THROW(e);
    StopAll();
  }
}

void DevcashController::ValidatorToyCallback(DevcashMessageUniquePtr ptr) {
  MTR_SCOPE_FUNC();
  LOG_DEBUG << "DevcashController::ValidatorToyCallback()";
  assert(ptr);
  if (validator_flipper_) {
    PushConsensus(std::move(ptr));
  }
  validator_flipper_ = !validator_flipper_;
}

void DevcashController::ConsensusToyCallback(DevcashMessageUniquePtr ptr) {
  MTR_SCOPE_FUNC();
  LOG_DEBUG << "DevcashController()::ConsensusToyCallback()";
  assert(ptr);
  if (consensus_flipper_) {
    server_.QueueMessage(std::move(ptr));
  }
  consensus_flipper_ = !consensus_flipper_;
}

std::vector<std::vector<byte>> DevcashController::GenerateTransactions() {
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
                         , getEpoch()+(1000000*(context_.get_current_node()+1)*(batch_counter+1))
          , keys_.getKey(inn_addr), keys_);
      std::vector<byte> inn_canon(inn_tx.getCanonical());
      batch.insert(batch.end(), inn_canon.begin(), inn_canon.end());
      LOG_DEBUG << "GenerateTransactions(): generated inn_tx with sig: " << toHex(inn_tx.getSignature());
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
                              , getEpoch()+(1000000*(context_.get_current_node()+1)*(i+1)*(j+1))
                              , keys_.getWalletKey(i), keys_);
          std::vector<byte> peer_canon(peer_tx.getCanonical());
          batch.insert(batch.end(), peer_canon.begin(), peer_canon.end());
          LOG_TRACE << "GenerateTransactions(): generated tx with sig: " << toHex(peer_tx.getSignature());
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

std::vector<std::vector<byte>> DevcashController::LoadTransactions() {
  LOG_DEBUG << "Loading transactions from " << scan_dir_;
  MTR_SCOPE_FUNC();
  std::vector<std::vector<byte>> out;

  ChainState priori;

  fs::path p(scan_dir_);

  if (!is_directory(p)) {
    LOG_ERROR << "Error opening dir: " << scan_dir_ << " is not a directory";
    return out;
  }

  size_t tx_count = 0;
  for(auto& entry : boost::make_iterator_range(fs::directory_iterator(p), {})) {
    LOG_DEBUG << "Reading " << entry;
    std::ifstream file(entry.path().string(), std::ios::binary);
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
    while (offset < file_size) {
      //constructor increments offset by reference
      FinalBlock one_block(raw, priori, offset);
      Summary sum(one_block.getSummary());
      Validation val(one_block.getValidation());
      std::pair<Address, Signature> pair(val.getFirstValidation());
      int index = keys_.getNodeIndex(pair.first);
      Tier1Transaction tx(sum, pair.second, (uint64_t) index, keys_);
      std::vector<byte> tx_canon(tx.getCanonical());
      batch.insert(batch.end(), tx_canon.begin(), tx_canon.end());
      tx_count++;
    }
    out.push_back(batch);
  }

  LOG_INFO << "Loaded " << std::to_string(tx_count) << " transactions in " << out.size() << " batches.";
  return out;
}

std::vector<byte> DevcashController::Start() {
  MTR_SCOPE_FUNC();
  std::vector<byte> out;
  CASH_TRY {

    auto lambda_callback = [this](DevcashMessageUniquePtr ptr) {
      if (ptr->message_type == TRANSACTION_ANNOUNCEMENT) {
          PushValidator(std::move(ptr));
        } else {
          PushConsensus(std::move(ptr));
        }
    };

    peer_client_.AttachCallback(lambda_callback);
    peer_client_.ListenTo("peer");
    peer_client_.ListenTo(context_.get_uri());

    loopback_client_.AttachCallback(lambda_callback);
    loopback_client_.ListenTo(context_.get_uri());

    server_.StartServer();
    peer_client_.StartClient();
    loopback_client_.StartClient();

    std::vector<std::vector<byte>> transactions;
    size_t processed = 0;

    workers_->Start();

    if (mode_ == eAppMode::T2 && generate_count_ > 0) {
      std::lock_guard<std::mutex> guard(mutex_);
      LOG_INFO << "Generate Transactions.";
      transactions = GenerateTransactions();
      LOG_INFO << "Finished Generating " << transactions.size() * batch_size_ << " Transactions.";
    } else if (mode_ == eAppMode::T1 && !scan_dir_.empty()) {
      transactions = LoadTransactions();
    } else {
      LOG_WARNING << "Not loading or generating: " << scan_dir_;
    }

    if (context_.get_sync_host().size() > 0) {
      io::synchronize(context_.get_sync_host()
        , (context_.get_current_node()%context_.get_peer_count()));
    } else {
      LOG_NOTICE << "DevcashController::Start(): Starting devcash - entering forever loop in 10 sec";
      sleep(1);
    }

    // Loop for long runs
    auto ms = kMAIN_WAIT_INTERVAL;
    while (true) {
      LOG_DEBUG << "Sleeping for " << ms
                << ": processed/batches/pending (" << processed
                << "/" << transactions.size() << "/"
                << utx_pool_.NumPendingTransactions() << ")";
      std::this_thread::sleep_for(millisecs(ms));

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
          server_.QueueMessage(std::move(blocks_msg1));
          std::string uri2 = context_.get_uri_from_index(context_.get_peer_count()*2+context_.get_current_node()%context_.get_peer_count());
          auto blocks_msg2 = std::make_unique<DevcashMessage>(uri2
                                                      , GET_BLOCKS_SINCE
                                                      , request
                                                      , remote_blocks_);
          server_.QueueMessage(std::move(blocks_msg2));
        } else if (mode_ == eAppMode::T2) {
          //a T2 should request new blocks from the T1 node with the same node index as itself
          std::string uri = context_.get_uri_from_index(context_.get_current_node()%context_.get_peer_count());
          auto blocks_msg = std::make_unique<DevcashMessage>(uri
                                                      , GET_BLOCKS_SINCE
                                                      , request
                                                      , remote_blocks_);
          server_.QueueMessage(std::move(blocks_msg));
        }
        remote_blocks_ = final_chain_.size();
      }

      if (processed < transactions.size()) {
        auto announce_msg = std::make_unique<DevcashMessage>(context_.get_uri()
                                                             , TRANSACTION_ANNOUNCEMENT
                                                             , transactions.at(processed)
                                                             , DEBUG_TRANSACTION_INDEX);
        server_.QueueMessage(std::move(announce_msg));
        processed++;
      } else if (!utx_pool_.HasPendingTransactions()) {
        if (final_chain_.getNumTransactions()
          >= (transactions.size()*batch_size_*context_.get_peer_count())) {
          LOG_INFO << "All transactions complete.  Shutting down";
          StopAll();
        } else if (mode_ == eAppMode::T1
          && final_chain_.getNumTransactions() >= transactions.size()) {
          LOG_INFO << "T1 transactions completed.  Shutting down.";
          StopAll();
        } else {
          LOG_INFO << "This node's transactions complete. Wait for peers to finish.";
		    }
      }

      if (shutdown_) break;
    }

    //write final chain to output
    std::vector<byte> final_chain_bin(final_chain_.BinaryDump());
    out.insert(out.end(), final_chain_bin.begin()
            , final_chain_bin.end());

  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashController.Start()");
    StopAll();
  }
  return out;
}

void DevcashController::StopAll() {
  CASH_TRY {
    LOG_DEBUG << "DevcashController::StopAll()";
    shutdown_ = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    peer_client_.StopClient();
    loopback_client_.StopClient();
    server_.StopServer();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    workers_->StopAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashController.StopAll()");
  }
}

void DevcashController::PushValidator(DevcashMessageUniquePtr ptr) {
  CASH_TRY {
    LOG_DEBUG << "DevcashController::PushValidator()";
    workers_->pushValidator(std::move(ptr));
  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashController.PushValidator()");
  }
}

void DevcashController::PushConsensus(DevcashMessageUniquePtr ptr) {
  CASH_TRY {
    LOG_DEBUG << "DevcashController::PushConsensus()";
    workers_->pushConsensus(std::move(ptr));
  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashController.PushValidator()");
  }
}

} //end namespace Devcash
