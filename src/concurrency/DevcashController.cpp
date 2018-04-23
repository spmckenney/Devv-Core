/*
 * DevcashController.cpp
 *
 *  Created on: Mar 23, 2018
 *      Author: Nick Williams
 *      Updated: Shawn McKenney <shawn.mckenney@emmion.com>
 */
#include "DevcashController.h"

#include <condition_variable>
#include <mutex>
#include <chrono>
#include <thread>
#include <string>

#include "DevcashWorker.h"
#include "common/devcash_context.h"
#include "common/util.h"
#include "io/message_service.h"
#include "consensus/finalblock.h"
#include "consensus/proposedblock.h"
#include "consensus/KeyRing.h"

typedef std::chrono::milliseconds millisecs;

namespace Devcash {

using namespace Devcash;

DevcashController::DevcashController(io::TransactionServer& server,
                                     io::TransactionClient& client,
                                     const int validatorCount,
                                     const int consensusWorkerCount,
                                     const int repeatFor,
                                     KeyRing& keys,
                                     DevcashContext& context)
  : server_(server)
  , client_(client)
  , validator_count_(validatorCount)
  , consensus_count_(consensusWorkerCount)
  , repeat_for_(repeatFor)
  , keys_(keys)
  , context_(context)
  , final_chain_("final_chain_")
  , proposed_chain_("proposed_chain_")
  , upcoming_chain_("upcoming_chain_")
  , seeds_at_(0)
  , workers_(new DevcashControllerWorker(this, validator_count_, consensus_count_))
{
  MTR_BEGIN_FUNC();
  keys_.initKeys();
  LOG_INFO << "Crypto Keys initialized.";

  ProposedPtr upcoming = std::make_shared<ProposedBlock>();
  upcoming_chain_.push_back(upcoming);
  LOG_INFO << "Upcoming chain created";

  proposal_closure_ = [this](unsigned int block_height) {
    MaybeCreateNextProposal(block_height,
                            this->context_,
                            this->keys_,
                            this->proposed_chain_,
                            this->upcoming_chain_,
                            this->pending_proposal_,
                            [this](DevcashMessageUniquePtr p) { this->server_.QueueMessage(std::move(p));});
  };
  MTR_END_FUNC();
}

std::string GetHighestMerkleRoot(const FinalBlockchain& final_chain) {
  MTR_SCOPE_FUNC();
  unsigned int block_height = final_chain.size();
  std::string prev_hash = "Genesis";
  if (block_height > 0) {
    prev_hash = final_chain.back()->hashMerkleRoot_;
    if (prev_hash == "") LOG_FATAL << "Previous block (#"
      +std::to_string(final_chain.back()->block_height_)+") Merkle missing!";
  }
  return prev_hash;
}

void DevcashController::ValidatorCallback(DevcashMessageUniquePtr ptr) {
  int prop_id = 1235;
  MTR_SCOPE_FUNC();
  LOG_DEBUG << "DevcashController::ValidatorCallback()";
  if (shutdown_) {
    return;
  }
  if (ptr->message_type == TRANSACTION_ANNOUNCEMENT) {
    MTR_START("ValidatorCallback", "transaction_anouncement", &prop_id);
    MTR_STEP("ValidatorCallback", "transaction_anouncement", &prop_id, "bin2Str");
    DevcashMessage msg(*ptr.get());
    std::string tx_str = bin2Str(msg.data);
    LOG_DEBUG << "New transaction: "+tx_str;
    MTR_STEP("ValidatorCallback", "transaction_anouncement", &prop_id, "addTransaction");
    upcoming_chain_.back()->addTransaction(tx_str, keys_);
    MTR_FINISH("ValidatorCallback", "transaction_anouncement", &prop_id);
  } else {
    LOG_DEBUG << "Unexpected message @ validator, to consensus.\n";
    PushConsensus(std::move(ptr));
  }
}

bool HandleFinalBlock(DevcashMessageUniquePtr ptr,
                      const DevcashContext& context,
                      const KeyRing& keys,
                      ProposedBlockchain& proposed_chain,
                      ProposedBlockchain& upcoming_chain,
                      FinalBlockchain& final_chain,
                      std::function<void(DevcashMessageUniquePtr)> callback) {
  int prop_id = 1234;
  MTR_SCOPE_FUNC();
  // Make highest proposed block final
  // check if propose next
  // if so, send proposal with all pending valid txs
  MTR_START("DevcashController","HandleFinalBlock", &prop_id);
  MTR_STEP("DevcashController", "HandleFinalBlock", &prop_id, "msg");
  DevcashMessage msg(*ptr.get());
  MTR_STEP("DevcashController", "HandleFinalBlock", &prop_id, "bin2Str");
  std::string final_block_str = bin2Str(msg.data);
  LOG_DEBUG << "Got final block: " + final_block_str;
  ProposedPtr highest_proposal = proposed_chain.back();
  MTR_STEP("DevcashController", "HandleFinalBlock", &prop_id, "new_block");
  DCBlock new_block(final_block_str, keys);
  new_block.SetBlockState(highest_proposal->GetBlockState());

  MTR_STEP("DevcashController", "HandleFinalBlock", &prop_id, "addValidation");
  highest_proposal->GetValidationBlock().addValidation(new_block.GetValidationBlock());
  MTR_STEP("DevcashController", "HandleFinalBlock", &prop_id, "finalize");
  highest_proposal->finalize(GetHighestMerkleRoot(final_chain));

  // Did we send a message
  bool sent_message = false;

  MTR_STEP("DevcashController", "HandleFinalBlock", &prop_id, "compare");
  if (highest_proposal->compare(new_block)) {
    // (nick@cs) the final block needs to have the validations in the order specified
    // by the remote node that finalized, so use final_block_str instead of highest_proposal
    FinalPtr top_block = FinalPtr(new FinalBlock(final_block_str,
                                                 highest_proposal->block_height_,
                                                 keys));

    top_block->copyHeaders(new_block);
    final_chain.push_back(top_block);
  } else {
    LOG_DEBUG << "Highest Proposal: "+highest_proposal->ToJSON();
    LOG_DEBUG << "Final block: "+new_block.ToJSON();
    LOG_FATAL << "Final block is inconsistent with chain.";
    // FIXME(spmckenney): handle LOG_FATAL and shutdown
    // StopAll();
  }
  MTR_FINISH("DevcashController", "HandleFinalBlock", &prop_id);
  return sent_message;
}

bool HandleProposalBlock(DevcashMessageUniquePtr ptr,
                         const DevcashContext& context,
                         const KeyRing& keys,
                         ProposedBlockchain& proposed_chain,
                         const ProposedBlockchain& upcoming_chain,
                         const FinalBlockchain& final_chain,
                         std::function<void(DevcashMessageUniquePtr)> callback) {
  int prop_id = 1234;
  MTR_SCOPE_FUNC();
  Timer timer;

  MTR_START("DevcashController","HandleProposalBlock", &prop_id);
  MTR_STEP("DevcashController", "HandleProposalBlock", &prop_id, "msg");
  bool sent_message = false;
  DevcashMessage msg(*ptr.get());
  LOG_TRACE << "timer -1 (" << timer() << ")";
  MTR_STEP("DevcashController", "HandleProposalBlock", &prop_id, "bin2str");
  std::string raw_str = bin2Str(msg.data);
  LOG_TRACE << "timer 0 (" << timer() << ")";
  LOG_DEBUG << "Received block proposal: " + raw_str;
  unsigned int block_height = final_chain.size();
  ProposedPtr next_proposal = upcoming_chain.at(block_height);
  MTR_STEP("DevcashController", "HandleProposalBlock", &prop_id, "new_proposal");
  ProposedPtr new_proposal = std::make_shared<ProposedBlock>(raw_str,
                                                             final_chain.size(),
                                                             keys);
  LOG_TRACE << "timer 1 (" << timer() << ")";
  new_proposal->SetBlockState(next_proposal->GetBlockState());
  MTR_STEP("DevcashController", "HandleProposalBlock", &prop_id, "ValidateBlock");
  if (new_proposal->validateBlock(keys)) {
    LOG_TRACE << "timer 2 (" << timer() << ")";
    LOG_DEBUG << "Proposed block is valid.";
    proposed_chain.push_back(new_proposal);
    MTR_STEP("DevcashController", "HandleProposalBlock", &prop_id, "signBlock");
    new_proposal->signBlock(keys.getNodeKey(context.get_current_node()),
                            context.kNODE_ADDRs[context.get_current_node()]);
    LOG_TRACE << "timer 3 (" << timer() << ")";
    int proposer = (proposed_chain.size() - 1) % context.get_peer_count();
    MTR_STEP("DevcashController", "HandleProposalBlock", &prop_id, "ToJSON");
    raw_str = new_proposal->GetValidationBlock().ToJSON();
    LOG_DEBUG << "Validation: " + raw_str;
    MTR_STEP("DevcashController", "HandleProposalBlock", &prop_id, "str2Bin");
    std::vector<uint8_t> data(str2Bin(raw_str));

    LOG_TRACE << "timer 4 (" << timer() << ")";
    MTR_STEP("DevcashController", "HandleProposalBlock", &prop_id, "get_uri_from_index");
    std::string remote_uri = context.get_uri_from_index(proposer);
    MTR_STEP("DevcashController", "HandleProposalBlock", &prop_id, "new_valid");
    auto valid = std::make_unique<DevcashMessage>(remote_uri,
                                                  VALID,
                                                  data);
    callback(std::move(valid));
    sent_message = true;
  } else {
    LOG_FATAL << "Proposed Block is invalid!\n"+
              new_proposal->ToJSON()+"\n----END of BLOCK-------\n";
  }
  LOG_DEBUG << "finished processing proposal";
  LOG_TRACE << "timer 5 (" << timer() << ")";
  MTR_FINISH("DevcashController", "HandleProposalBlock", &prop_id);
  return sent_message;
}

bool HandleValidationBlock(DevcashMessageUniquePtr ptr,
                           const DevcashContext& context,
                           const KeyRing& keys,
                           const ProposedBlockchain& proposed_chain,
                           ProposedBlockchain& upcoming_chain,
                           FinalBlockchain& final_chain,
                           std::function<void(DevcashMessageUniquePtr)> callback) {
  MTR_BEGIN_FUNC();
  bool sent_message = false;
  //increment validation count
  //if count >= validationPercent, finalize block
  DevcashMessage msg(*ptr.get());
  std::string rawVal = bin2Str(msg.data);
  LOG_DEBUG << "Received block validation: " + rawVal;
  unsigned int block_height = final_chain.size();

  LOG_INFO << "HandleValidationBlock(): " << block_height << " % "
           << context.get_peer_count() << " (" << block_height % context.get_peer_count()
           << ") current(" << context.get_current_node() << ")";

  if (block_height % context.get_peer_count() != context.get_current_node()) {
    LOG_WARNING << "Got a VALID message, but this node did not propose!";
    return sent_message;
  }

  DCValidationBlock validation(rawVal);
  ProposedBlock highest_proposal = *proposed_chain.back().get();
  highest_proposal.GetValidationBlock().addValidation(validation);
  if (highest_proposal.GetValidationBlock().GetValidationCount() > 1) {
    highest_proposal.finalize(GetHighestMerkleRoot(final_chain));
    FinalPtr top_block =std::make_shared<FinalBlock>(highest_proposal
                        , highest_proposal.block_height_);
    top_block->copyHeaders(highest_proposal);

    final_chain.push_back(top_block);

    ProposedPtr upcoming = std::make_shared<ProposedBlock>(""
                           , upcoming_chain.size(), keys);
    upcoming->SetBlockState(proposed_chain.back()->GetBlockState());
    upcoming_chain.push_back(upcoming);

    std::string final_str = top_block->ToJSON();
    LOG_DEBUG << "Final block: "+final_str;
    std::vector<uint8_t> data(str2Bin(final_str));
    auto finalBlock = std::make_unique<DevcashMessage>("peers", FINAL_BLOCK, data);
    callback(std::move(finalBlock));
    sent_message = true;
  } else {
    unsigned int vals = proposed_chain.back()->GetValidationBlock().GetValidationCount();
    LOG_INFO << "Block proposal validated "+std::to_string(vals)+" times.\n";
  }
  MTR_END_FUNC();
  return sent_message;
}

/**
 * ConsensusCallback
 *
 * Master callback to handle incoming messages requiring consensus
 */
void DevcashController::ConsensusCallback(DevcashMessageUniquePtr ptr) {
  MTR_BEGIN_FUNC();
  LOG_DEBUG << "DevcashController()::ConsensusCallback()";
  if (shutdown_) {
    MTR_END_FUNC();
    return;
  }
  // FIXME(spmckenney): I removed the response timeout logic
  // TODO: Add message response timeout logic

  // Start trace timer
  Timer timer;
  switch (ptr->message_type) {
    case (eMessageType::FINAL_BLOCK) : {
      LOG_TRACE << "DevcashController()::ConsensusCallback(): FINAL_BLOCK begin";
      auto res = HandleFinalBlock(std::move(ptr),
                                  context_,
                                  keys_,
                                  proposed_chain_,
                                  upcoming_chain_,
                                  final_chain_,
                                  [this](DevcashMessageUniquePtr p) { this->server_.QueueMessage(std::move(p));});
      LOG_TRACE << "DevcashController()::ConsensusCallback(): FINAL_BLOCK complete: " << timer();
      proposal_closure_(final_chain_.size());
      LOG_TRACE << "DevcashController()::ConsensusCallback(): PROPOSAL_CLOSURE complete: " << timer();
      break;
    }
    case (eMessageType::PROPOSAL_BLOCK) : {
      LOG_TRACE << "DevcashController()::ConsensusCallback(): PROPOSAL_BLOCK begin";
      auto res = HandleProposalBlock(std::move(ptr),
                                     context_,
                                     keys_,
                                     proposed_chain_,
                                     upcoming_chain_,
                                     final_chain_,
                                     [this](DevcashMessageUniquePtr p) { this->server_.QueueMessage(std::move(p));});
      waiting_ = 0;
      LOG_TRACE << "DevcashController()::ConsensusCallback(): PROPOSAL_BLOCK complete: " << timer();
      break;
    }
    case (eMessageType::TRANSACTION_ANNOUNCEMENT) : {
      LOG_TRACE << "DevcashController()::ConsensusCallback(): TRANSACTION_ANNOUNCEMENT begin";
      LOG_DEBUG << "Unexpected message @ consensus, to validator";
      PushValidator(std::move(ptr));
      LOG_TRACE << "DevcashController()::ConsensusCallback(): TRANSACTION_ANNOUNCEMENT complete: << timer()";
      break;
    }
    case (eMessageType::VALID) : {
      LOG_TRACE << "DevcashController()::ConsensusCallback(): VALIDATION (" << pending_proposal_.load() << ") begin";
      std::lock_guard<std::mutex> lock(valid_lock_);

      LOG_TRACE << "DevcashController()::ConsensusCallback(): VALIDATION ("
                << pending_proposal_.load() << ") lock acquired: "<< timer();

      // If we have a proposed block pending
      if (pending_proposal_.load()) {
        auto res = HandleValidationBlock(std::move(ptr),
                                         context_,
                                         keys_,
                                         proposed_chain_,
                                         upcoming_chain_,
                                         final_chain_,
                                         [this](DevcashMessageUniquePtr p) { this->server_.QueueMessage(std::move(p));});
        pending_proposal_.store(false);
      }
      LOG_TRACE << "DevcashController()::ConsensusCallback(): VALIDATION ("
                << pending_proposal_.load() << ") complete: " << timer();
      break;
    }
    case (eMessageType::REQUEST_BLOCK) : {
      LOG_DEBUG << "DevcashController()::ConsensusCallback(): REQUEST_BLOCK";
      // provide blocks since requested height
      break;
    }
    default : {
      LOG_ERROR << "DevcashController()::ConsensusCallback(): Unexpected message, ignore.\n";
    }
  }
  MTR_END_FUNC();
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

void DevcashController::SeedTransactions(std::string txs) {
  MTR_BEGIN_FUNC();
  CASH_TRY {
    std::string toParse(txs);
    toParse.erase(std::remove(toParse.begin(), toParse.end(), '\n'),
        toParse.end());
    toParse.erase(std::remove(toParse.begin(), toParse.end(), '\r'),
        toParse.end());
    int counter=0;
    if (toParse.at(0) == '{') {
      size_t dex = toParse.find("[[")+1;
      size_t eDex = dex;
      while (eDex < toParse.size()-2) {
        size_t eDex = toParse.find("}],[{", dex);
        counter++;
        if (eDex == std::string::npos) {
          eDex = toParse.find("}]]");
          if (eDex == std::string::npos) {
            LOG_FATAL << "Invalid input.";
            StopAll();
          }
          std::string txSubstr(toParse.substr(dex, eDex-dex+2));
          while (repeat_for_ > 0) {
            repeat_for_--;
            PostAdvanceTransactions(txSubstr);
            seeds_.push_back(txSubstr);
          }
          break;
        } else {
          std::string txSubstr(toParse.substr(dex, eDex-dex+2));
          seeds_.push_back(txSubstr);
          PostAdvanceTransactions(txSubstr);
          dex = toParse.find("[", eDex);
        }
      }
      LOG_INFO << "Seeded input for "+std::to_string(seeds_.size())+" blocks.";
      //postTransactions();
    } else {
      LOG_FATAL << "Input has wrong syntax!";
      StopAll();
    }
  } CASH_CATCH (const std::exception& e) {
    LOG_ERROR << FormatException(&e, "DevcashController.SeedTransactions");
  }
  MTR_END_FUNC();
}

bool DevcashController::PostAdvanceTransactions(const std::string& inputTxs) {
  MTR_BEGIN_FUNC();
  CASH_TRY {
    int counter = 0;
    LOG_DEBUG << "Posting block height "+std::to_string(seeds_at_);
    if (upcoming_chain_.size()-1 < seeds_at_) {
      ProposedPtr next_proposal = upcoming_chain_.back();
      ProposedPtr upcoming_ptr = std::make_shared<ProposedBlock>(""
              , upcoming_chain_.size(), keys_);
          upcoming_ptr->SetBlockState(next_proposal->GetBlockState());
          upcoming_chain_.push_back(upcoming_ptr);
    }
    ProposedPtr upcoming = upcoming_chain_.at(seeds_at_);
    seeds_at_++;
    size_t dex = inputTxs.find("\""+kOPER_TAG+"\":", 0);
    size_t eDex = inputTxs.find(kSIG_TAG, dex);
    eDex = inputTxs.find("}", eDex);
    std::string oneTx = inputTxs.substr(dex-1, eDex-dex+2);
    upcoming->addTransaction(oneTx, keys_);
    counter++;
    while (inputTxs.at(eDex+1) != ']' && eDex < inputTxs.size()-2) {
      dex = inputTxs.find("{", eDex);
      eDex = inputTxs.find(kSIG_TAG, dex);
      eDex = inputTxs.find("}", eDex);
      oneTx = inputTxs.substr(dex, eDex-dex+1);
      LOG_DEBUG << "One tx: "+oneTx;
      upcoming->addTransaction(oneTx, keys_);
      counter++;
    }
    ProposedPtr next_proposal = upcoming_chain_.back();
    unsigned int block_height = upcoming_chain_.size();
    LOG_INFO << "POST Upcoming #"+std::to_string(block_height)+" has "
        +std::to_string(next_proposal->vtx_.size())+" transactions.";
    LOG_DEBUG << std::to_string(counter)+" transactions posted upcoming.";
    MTR_END_FUNC();
    return true;
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "DevcashController.postTransactions");
  }
  MTR_END_FUNC();
  return false;
}

bool DevcashController::PostTransactions() {
  MTR_BEGIN_FUNC();
  CASH_TRY {
    unsigned int upcoming_height = upcoming_chain_.size();
    LOG_DEBUG << "Seed block height "+std::to_string(seeds_at_)+
        " ready for height: "+std::to_string(upcoming_height-1);
    if (seeds_at_ > upcoming_height) return true;
    int counter = 0;
    if (seeds_.size() > upcoming_height) {
      LOG_DEBUG << "Posting block height "+std::to_string(seeds_at_);
      std::string someTxs = seeds_.at(seeds_at_);
      ProposedPtr upcoming = upcoming_chain_.at(seeds_at_);
      seeds_at_++;
      size_t dex = someTxs.find("\""+kOPER_TAG+"\":", 0);
      size_t eDex = someTxs.find(kSIG_TAG, dex);
      eDex = someTxs.find("}", eDex);
      std::string oneTx = someTxs.substr(dex-1, eDex-dex+2);
      upcoming->addTransaction(oneTx, keys_);
      counter++;
      while (someTxs.at(eDex+1) != ']' && eDex < someTxs.size()-2) {
        dex = someTxs.find("{", eDex);
        eDex = someTxs.find(kSIG_TAG, dex);
        eDex = someTxs.find("}", eDex);
        oneTx = someTxs.substr(dex, eDex-dex+1);
        LOG_DEBUG << "One tx: "+oneTx;
        upcoming->addTransaction(oneTx, keys_);
        counter++;
      }
      ProposedPtr next_proposal = upcoming_chain_.back();
      unsigned int block_height = final_chain_.size();
      LOG_INFO << "POST Upcoming #"+std::to_string(block_height)+" has "
          +std::to_string(next_proposal->vtx_.size())+" transactions.";
      LOG_DEBUG << std::to_string(counter)+" transactions posted upcoming.";
    } else { //all input processed by the chain
      MTR_END_FUNC();
      return false;
    }
    MTR_END_FUNC();
    return true;
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "DevcashController.PostTransactions");
  }
  MTR_END_FUNC();
  return false;
}

void DevcashController::StartToy(unsigned int node_index) {
  workers_->start();

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
  workers_->start();

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

  LOG_INFO << "Starting a control sleep";
  sleep(2);

  if ((final_chain_.size() % context_.get_peer_count()) == context_.get_current_node()) {
    LOG_INFO << "This node's turn to create proposal.";
    MaybeCreateNextProposal(final_chain_.size(),
                            context_,
                            keys_,
                            proposed_chain_,
                            upcoming_chain_,
                            pending_proposal_,
                            [this](DevcashMessageUniquePtr p) { this->server_.QueueMessage(std::move(p));});

  } else if (waiting_ == 0) {
    millisecs ms =
      std::chrono::duration_cast<millisecs>(std::chrono::system_clock::now().time_since_epoch());
    waiting_ = ms.count();
  } else {
    millisecs ms =
      std::chrono::duration_cast<millisecs>(std::chrono::system_clock::now().time_since_epoch());
    if (ms.count() > static_cast<int>(waiting_ + kPROPOSAL_TIMEOUT)) {
      LOG_FATAL << "Proposal timed out at block #"
        +std::to_string(final_chain_.size());
      StopAll();
    }
  }

  // Loop for long runs
  bool transactions_to_post = PostTransactions();
  auto ms = kMAIN_WAIT_INTERVAL;
  while (true) {
    LOG_DEBUG << "Sleeping for " << ms;
    std::this_thread::sleep_for(millisecs(ms));
    if (transactions_to_post)
      transactions_to_post = PostTransactions();
    if (final_chain_.size() >= seeds_.size()-1) {
      LOG_WARNING << "final_chain_.size() >= seeds_.size(), break()ing";
      break;
    }
    if (shutdown_) break;
  }

  for(size_t i=0; i < final_chain_.size(); ++i) {
    out += final_chain_.at(i)->ToJSON();
  }

  return out;
}

void DevcashController::StopAll() {
  LOG_DEBUG << "DevcashController::StopAll()";
  shutdown_ = true;
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  client_.StopClient();
  server_.StopServer();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  workers_->StopAll();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void DevcashController::PushValidator(DevcashMessageUniquePtr ptr) {
  MTR_BEGIN_FUNC();
  LOG_DEBUG << "DevcashController::PushValidator()";
  workers_->pushValidator(std::move(ptr));
  MTR_END_FUNC();
}

void DevcashController::PushConsensus(DevcashMessageUniquePtr ptr) {
  LOG_DEBUG << "DevcashController::PushConsensus()";
  workers_->pushConsensus(std::move(ptr));
}

void MaybeCreateNextProposal(unsigned int block_height,
                             const DevcashContext& context,
                             const KeyRing& keys,
                             ProposedBlockchain& proposed_chain,
                             ProposedBlockchain& upcoming_chain,
                             std::atomic<bool>& pending_proposal,
                             std::function<void(DevcashMessageUniquePtr)> callback) {
  int prop_id = 1234;
  MTR_SCOPE_FUNC();
  if ((block_height % context.get_peer_count()) == context.get_current_node()) {
    LOG_INFO << "This node's turn to create proposal.";
  } else {
    LOG_INFO << "Not this node's turn to create proposal.";
    return;
  }
  MTR_START("DevcashController", "MaybeCreateNextProposal", &prop_id);
  MTR_STEP("DevcashController", "MaybeCreateNextProposal", &prop_id, "loading");
  if (pending_proposal.load()) {
    LOG_ERROR << "Won't create proposal, pending_proposal == true";
  }

  MTR_STEP("DevcashController", "MaybeCreateNextProposal", &prop_id, "logging");
  LOG_TRACE << "DevcashController()::CreateNextProposal(" << block_height << "): begin";
  Timer timer;

  auto next_proposal = *upcoming_chain.at(block_height);

  LOG_INFO << "Upcoming #"+std::to_string(block_height)+" has "
    +std::to_string(next_proposal.vtx_.size())+" transactions.";

  if (!(block_height % 100) || !((block_height + 1) % 100)) {
    LOG_WARNING << "Processing @ final_chain_.size: (" << std::to_string(block_height) << ")";
  }

  MTR_STEP("DevcashController", "MaybeCreateNextProposal", &prop_id, "create upcoming");
  // (nick@cs) we need to create a new block in the upcoming chain that accepts
  // incoming transactions from other validation workers before we copy the
  // previous back() of upcoming_chain into proposed_chain
  // In fact, this should happen before we get the next_proposal reference
  // in case a validator worker is currently adding a transaction to it.
  ProposedPtr upcoming_ptr = std::make_shared<ProposedBlock>(""
    , block_height+1, keys);
  upcoming_ptr->SetBlockState(next_proposal.GetBlockState());

  upcoming_chain.push_back(upcoming_ptr);

  static int counter = 0;
  MTR_COUNTER("DevcashController", "CreateProposalCounter", counter);

  MTR_STEP("DevcashController", "MaybeCreateNextProposal", &prop_id, "create proposed");

  // (spmckenney) I'm not sure why proposal_pushed onto the proposed_chain_
  // and then immediately popped into new pointer...
  ProposedPtr proposal_ptr = std::make_shared<ProposedBlock>(next_proposal.vtx_,
                                                             next_proposal.GetValidationBlock(),
                                                             next_proposal.block_height_);

  LOG_INFO << "Proposal #" + std::to_string(block_height) + " has "
    + std::to_string(proposal_ptr->vtx_.size()) + " transactions.";

  MTR_STEP("DevcashController", "MaybeCreateNextProposal", &prop_id, "validate");
  // Validate and sign
  proposal_ptr->validate(keys);
  MTR_STEP("DevcashController", "MaybeCreateNextProposal", &prop_id, "sign");
  proposal_ptr->signBlock(keys.getNodeKey(context.get_current_node()),
                      context.kNODE_ADDRs[context.get_current_node()]);

  MTR_STEP("DevcashController", "MaybeCreateNextProposal", &prop_id, "ToJSON");
  std::string proposal_str = proposal_ptr->ToJSON();
  MTR_STEP("DevcashController", "MaybeCreateNextProposal", &prop_id, "str2bin");

  proposed_chain.push_back(proposal_ptr);
  LOG_DEBUG << "Propose Block: "+proposal_str;
  std::vector<uint8_t> data(str2Bin(proposal_str));

  MTR_STEP("DevcashController", "MaybeCreateNextProposal", &prop_id, "create message");
  // Create message
  auto propose_msg = std::make_unique<DevcashMessage>("peers", PROPOSAL_BLOCK, data);
  LOG_TRACE << "DevcashController()::CreateNextProposal(): complete: " << timer();
  MTR_STEP("DevcashController", "MaybeCreateNextProposal", &prop_id, "callback");
  callback(std::move(propose_msg));
  MTR_STEP("DevcashController", "MaybeCreateNextProposal", &prop_id, "store");
  pending_proposal.store(true);
  MTR_FINISH("DevcashController", "MaybeCreateNextProposal", &prop_id);
  return;
}

} //end namespace Devcash
