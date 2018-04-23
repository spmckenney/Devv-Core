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

#include "common/devcash_context.h"
#include "common/util.h"
#include "io/message_service.h"
#include "consensus/KeyRing.h"

typedef std::chrono::milliseconds millisecs;

namespace Devcash {


DevcashController::DevcashController(
    io::TransactionServer& server,
    io::TransactionClient& client,
    const int validatorCount,
    const int consensusCount,
    KeyRing& keys,
    DevcashContext& context,
    const ChainState& prior)
  : server_(server)
  , client_(client)
  , validator_count_(validatorCount)
  , consensus_count_(consensusCount)
  , keys_(keys)
  , context_(context)
  , final_chain_("final_chain_")
  , utx_pool_(prior)
  //, workers_(new DevcashControllerWorker(this, validator_count_, consensus_count_))
{
  keys_.initKeys();
  LOG_INFO << "Crypto Keys initialized.";
}

DevcashMessageUniquePtr CreateNextProposal(const KeyRing& keys,
                        Blockchain& final_chain,
                        UnrecordedTransactionPool& utx_pool,
                        const DevcashContext& context) {
  size_t block_height = final_chain.size();

  LOG_TRACE << "DevcashController()::CreateNextProposal(): begin";

  size_t next_proposal_height = block_height+
      ((block_height+1)%context.get_peer_count());

  if (!(block_height % 100) || !((block_height + 1) % 100)) {
    LOG_WARNING << "Processing @ final_chain_.size: (" << std::to_string(block_height) << ")";
  }

  LOG_INFO << "Proposal #"+std::to_string(next_proposal_height)+".";

  Hash prev_hash = final_chain.getHighestMerkleRoot();
  ChainState prior = final_chain.getHighestChainState();
  std::vector<byte> proposal(utx_pool.ProposeBlock(prev_hash
      , prior, keys, context));

  LOG_DEBUG << "Propose Block: "+toHex(proposal);

  // Create message
  auto propose_msg = std::make_unique<DevcashMessage>("peers", PROPOSAL_BLOCK, proposal);
  LOG_TRACE << "DevcashController()::CreateNextProposal(): complete";
  return propose_msg;
}

void DevcashController::ValidatorCallback(DevcashMessageUniquePtr ptr) {
  LOG_DEBUG << "DevcashController::ValidatorCallback()";
  if (shutdown_) return;
  if (ptr->message_type == TRANSACTION_ANNOUNCEMENT) {
    DevcashMessage msg(*ptr.get());
    utx_pool_.AddTransactions(msg.data, keys_);
  } else {
    LOG_DEBUG << "Unexpected message @ validator, to consensus.\n";
    PushConsensus(std::move(ptr));
  }
}

bool HandleFinalBlock(DevcashMessageUniquePtr ptr,
                      const DevcashContext& context,
                      const KeyRing& keys,
                      Blockchain& final_chain,
                      UnrecordedTransactionPool& utx_pool,
                      std::function<void(DevcashMessageUniquePtr)> callback) {
  //Make the incoming block final
  //if pending proposal, makes sure it is still valid
  //if no pending proposal, check if should make one
  DevcashMessage msg(*ptr.get());
  LOG_DEBUG << "Got final block: " + toHex(msg.data);
  ChainState prior = final_chain.getHighestChainState();
  FinalPtr top_block = FinalPtr(new FinalBlock(utx_pool.FinalizeRemoteBlock(
      msg.data, prior, keys)));
  final_chain.push_back(top_block);
  LOG_TRACE << "final_chain.push_back()";

  // Did we send a message
  bool sent_message = false;

  if (utx_pool.HasProposal()) {
    ChainState current = top_block->getChainState();

    //ProposedBlock is still valid in new context, do nothing
    if (utx_pool.ReverifyProposal(current, keys)) return sent_message;

    //ProposedBlock is now invalid, make a new one
  } // else there is no ProposedBlock, make a new one

  if (!utx_pool.HasPendingTransactions()) {
    LOG_INFO << "All pending transactions processed.";
  } else {
    callback(std::move(CreateNextProposal(keys,final_chain,utx_pool,context)));
    sent_message = true;
  }
  return sent_message;
}

bool HandleProposalBlock(DevcashMessageUniquePtr ptr,
                         const DevcashContext& context,
                         const KeyRing& keys,
                         Blockchain& final_chain,
                         std::function<void(DevcashMessageUniquePtr)> callback) {
  //validate block
  //if valid, push VALID message
  bool sent_message = false;
  DevcashMessage msg(*ptr.get());
  LOG_DEBUG << "Received block proposal: " + toHex(msg.data);
  ChainState prior = final_chain.getHighestChainState();
  ProposedBlock to_validate(msg.data, prior, keys);
  if (!to_validate.validate(keys)) {
    LOG_WARNING << "ProposedBlock is invalid!";
    return sent_message;
  }
  if (!to_validate.SignBlock(keys, context)) {
    LOG_WARNING << "ProposedBlock.SignBlock failed!";
    return sent_message;
  }
  LOG_DEBUG << "Proposed block is valid.";
  std::vector<byte> validation(to_validate.getValidationData());
  LOG_DEBUG << "Validation: " + toHex(validation);
  auto valid = std::make_unique<DevcashMessage>("peers",
                                                VALID,
                                                validation);
  callback(std::move(valid));
  sent_message = true;
  LOG_DEBUG << "finished processing proposal";
  return sent_message;
}

bool HandleValidationBlock(DevcashMessageUniquePtr ptr,
                           const DevcashContext& context,
                           const KeyRing& keys,
                           Blockchain& final_chain,
                           UnrecordedTransactionPool& utx_pool,
                           std::function<void(DevcashMessageUniquePtr)> callback) {
  bool sent_message = false;
  DevcashMessage msg(*ptr.get());
  LOG_DEBUG << "Received block validation: " + toHex(msg.data);

  if (utx_pool.CheckValidation(msg.data, context)) {
    //block can be finalized, so finalize
    FinalPtr top_block = FinalPtr(new FinalBlock(utx_pool.FinalizeLocalBlock(
        keys)));
    final_chain.push_back(top_block);
    LOG_TRACE << "final_chain.push_back()";

    std::vector<byte> final_msg = top_block->getCanonical();
    LOG_DEBUG << "Final block: "+toHex(final_msg);

    auto finalBlock = std::make_unique<DevcashMessage>("peers", FINAL_BLOCK, final_msg);
    callback(std::move(finalBlock));
    sent_message = true;
  } else if (!utx_pool.HasProposal() && utx_pool.HasPendingTransactions()) {
    //no proposal, but transactions available, so make proposal
    callback(std::move(CreateNextProposal(keys,final_chain,utx_pool,context)));
    sent_message = true;
  } //else proposal exists, but not ready to finalize
    //or no pending transactions
    //do nothing in either case

  return sent_message;
}

void DevcashController::ConsensusCallback(DevcashMessageUniquePtr ptr) {
  LOG_DEBUG << "DevcashController()::ConsensusCallback()";
  if (shutdown_) return;
  if (ptr->message_type == FINAL_BLOCK) {
    LOG_TRACE << "DevcashController()::ConsensusCallback(): FINAL_BLOCK begin";
    HandleFinalBlock(std::move(ptr),
                                context_,
                                keys_,
                                final_chain_,
                                utx_pool_,
                                [this](DevcashMessageUniquePtr p) { this->server_.QueueMessage(std::move(p));});
    LOG_TRACE << "DevcashController()::ConsensusCallback(): FINAL_BLOCK complete";
  } else if (ptr->message_type == PROPOSAL_BLOCK) {
    LOG_TRACE << "DevcashController()::ConsensusCallback(): PROPOSAL_BLOCK begin";
    HandleProposalBlock(std::move(ptr),
                                   context_,
                                   keys_,
                                   final_chain_,
                                   [this](DevcashMessageUniquePtr p) { this->server_.QueueMessage(std::move(p));});
    waiting_ = 0;
    LOG_TRACE << "DevcashController()::ConsensusCallback(): PROPOSAL_BLOCK complete";
  } else if (ptr->message_type == TRANSACTION_ANNOUNCEMENT) {
    LOG_TRACE << "DevcashController()::ConsensusCallback(): TRANSACTION_ANNOUNCEMENT begin";
    LOG_DEBUG << "Unexpected message @ consensus, to validator";
    PushValidator(std::move(ptr));
    LOG_TRACE << "DevcashController()::ConsensusCallback(): TRANSACTION_ANNOUNCEMENT complete";
  } else if (ptr->message_type == VALID) {
    LOG_TRACE << "DevcashController()::ConsensusCallback(): VALIDATION begin";
    HandleValidationBlock(std::move(ptr),
                                     context_,
                                     keys_,
                                     final_chain_,
                                     utx_pool_,
                                     [this](DevcashMessageUniquePtr p) { this->server_.QueueMessage(std::move(p));});
    LOG_TRACE << "DevcashController()::ConsensusCallback(): VALIDATION complete";
  } else if (ptr->message_type == REQUEST_BLOCK) {
    LOG_DEBUG << "DevcashController()::ConsensusCallback(): REQUEST_BLOCK";
    //provide blocks since requested height
  } else {
    LOG_ERROR << "DevcashController()::ConsensusCallback(): Unexpected message, ignore.\n";
  }
}

void DevcashController::ValidatorToyCallback(DevcashMessageUniquePtr ptr) {
  LOG_DEBUG << "DevcashController::ValidatorToyCallback()";
  assert(ptr);
  if (validator_flipper_) {
    PushConsensus(std::move(ptr));
  }
  validator_flipper_ = !validator_flipper_;
}

void DevcashController::ConsensusToyCallback(DevcashMessageUniquePtr ptr) {
  LOG_DEBUG << "DevcashController()::ConsensusToyCallback()";
  assert(ptr);
  if (consensus_flipper_) {
    server_.QueueMessage(std::move(ptr));
  }
  consensus_flipper_ = !consensus_flipper_;
}

void DevcashController::SeedTransactions(std::vector<byte> txs) {
  CASH_TRY {

    utx_pool_.AddTransactions(txs, keys_);
  } CASH_CATCH (const std::exception& e) {
    LOG_ERROR << FormatException(&e, "DevcashController.SeedTransactions");
  }
}

std::vector<byte> DevcashController::GenerateTransactions(size_t num) {
  std::vector<byte> txs;
  EVP_MD_CTX* ctx;
  if(!(ctx = EVP_MD_CTX_create())) {
    LOG_FATAL << "Could not create signature context!";
    CASH_THROW("Could not create signature context!");
  }

  std::vector<byte> inn_bin(Hex2Bin(context_.kINN_ADDR));
  Address inn_addr;
  std::copy_n(inn_bin.begin(), kADDR_SIZE, inn_addr.begin());
  inn_addr[0] = 114;

  size_t addr_count = context_.kADDRs.size();
  std::vector<Address> addrs;
  for (size_t i=0; i<addr_count; ++i) {
    std::vector<byte> addr_bin(Hex2Bin(context_.kADDRs[i]));
    Address addr;
    std::copy_n(addr_bin.begin(), kADDR_SIZE, addr.begin());
    addrs.push_back(addr);
  }

  size_t counter = 0;
  while (counter < num) {
    std::vector<Transfer> xfers;
    Transfer inn_transfer(inn_addr, 0, -1*addr_count, 0);
    xfers.push_back(inn_transfer);
    for (size_t i=0; i<addr_count; ++i) {
      Transfer transfer(addrs.at(i), 0, 1, 0);
      xfers.push_back(transfer);
    }
    Transaction inn_tx(eOpType::Create, xfers, getEpoch()
        , keys_.getKey(inn_addr), keys_);
    std::vector<byte> inn_canon(inn_tx.getCanonical());
    txs.insert(txs.end(), inn_canon.begin(), inn_canon.end());
    counter++;
    for (size_t i=0; i<addr_count; ++i) {
      for (size_t j=0; j<addr_count; ++j) {
        if (i==j) continue;
        std::vector<Transfer> peer_xfers;
        Transfer sender(addrs.at(i), 0, -1, 0);
        peer_xfers.push_back(sender);
        Transfer receiver(addrs.at(j), 0, 1, 0);
        peer_xfers.push_back(receiver);
        Transaction peer_tx(eOpType::Create, peer_xfers, getEpoch()
            , keys_.getKey(addrs.at(i)), keys_);
        std::vector<byte> peer_canon(peer_tx.getCanonical());
        txs.insert(txs.end(), peer_canon.begin(), peer_canon.end());
        counter++;
      }
    }
  }

  LOG_INFO << "Generated "+std::to_string(counter)+" transactions.";
  return txs;
}

void DevcashController::StartToy(unsigned int node_index) {
  //workers_->start();

  LOG_DEBUG << "READY? StartToy()";
  client_.AttachCallback([this](DevcashMessageUniquePtr ptr) {
      if (ptr->message_type == TRANSACTION_ANNOUNCEMENT) {
          PushValidator(std::move(ptr));
        } else {
          PushConsensus(std::move(ptr));
        }
    });

  server_.StartServer();
  client_.StartClient();


  std::string uri = "RemoteURI-" + std::to_string(node_index);
  client_.ListenTo(uri);
  std::string peer = "peer";
  client_.ListenTo(peer);

  sleep(10);

  for (;;) {
    std::vector<uint8_t> data(100);
    auto startMsg = std::make_unique<DevcashMessage>(uri,
                                                     TRANSACTION_ANNOUNCEMENT,
                                                     data);
    server_.QueueMessage(std::move(startMsg));
    sleep(10);
  }
}

std::string DevcashController::Start() {
  std::string out;

  client_.AttachCallback([this](DevcashMessageUniquePtr ptr) {
    if (ptr->message_type == TRANSACTION_ANNOUNCEMENT) {
        PushValidator(std::move(ptr));
      } else {
        PushConsensus(std::move(ptr));
      }
  });

  client_.ListenTo("peer");
  client_.ListenTo(context_.get_uri());

  server_.StartServer();
  client_.StartClient();

  //TODO: fix inn trnsactions
  /*std::vector<byte> transactions = GenerateTransactions(1000);
  auto announce_msg = std::make_unique<DevcashMessage>(context_.get_uri()
      , TRANSACTION_ANNOUNCEMENT, transactions);
  server_.QueueMessage(std::move(announce_msg));*/

  //TODO: fix workers
  //workers_->Start();
  LOG_INFO << "Starting a control sleep";
  sleep(2);

  //TODO: fix status access violation
  /*server_.QueueMessage(std::move(CreateNextProposal(keys_,
      final_chain_,
      utx_pool_,
      context_)));

  // Loop for long runs
  auto ms = kMAIN_WAIT_INTERVAL;
  while (true) {
    LOG_DEBUG << "Sleeping for " << ms;
    std::this_thread::sleep_for(millisecs(ms));
    if (!utx_pool_.HasPendingTransactions()) {
      LOG_INFO << "No pending transactions.  Shut down.";
      StopAll();
    }
    if (shutdown_) break;
  }*/
  return out;
}

void DevcashController::StopAll() {
  LOG_DEBUG << "DevcashController::StopAll()";
  shutdown_ = true;
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  client_.StopClient();
  server_.StopServer();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  //workers_->StopAll();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void DevcashController::PushValidator(DevcashMessageUniquePtr ptr) {
  LOG_DEBUG << "DevcashController::PushValidator()";
  //workers_->pushValidator(std::move(ptr));
}

void DevcashController::PushConsensus(DevcashMessageUniquePtr ptr) {
  LOG_DEBUG << "DevcashController::PushConsensus()";
  //workers_->pushConsensus(std::move(ptr));
}

} //end namespace Devcash
