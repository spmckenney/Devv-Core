/*
 * DevcashController.cpp
 *
 *  Created on: Mar 23, 2018
 *      Author: Nick Williams
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
  keys_.initKeys();
  LOG_INFO << "Crypto Keys initialized.";

  ProposedPtr upcoming = std::make_shared<ProposedBlock>();
  LOG_TRACE << "upcoming_chain.push_back()";
  upcoming_chain_.push_back(upcoming);
  LOG_INFO << "Upcoming chain created";
}

std::string GetHighestMerkleRoot(const FinalBlockchain& final_chain) {
  unsigned int block_height = final_chain.size();
  std::string prev_hash = "Genesis";
  if (block_height > 0) {
    prev_hash = final_chain.back()->hashMerkleRoot_;
    if (prev_hash == "") LOG_FATAL << "Previous block (#"
      +std::to_string(final_chain.back()->block_height_)+") Merkle missing!";
  }
  return prev_hash;
}

DevcashMessageUniquePtr CreateNextProposal(unsigned int block_height,
                        const DevcashContext& context,
                        const KeyRing& keys,
                        ProposedBlockchain& proposed_chain,
                        ProposedBlockchain& upcoming_chain) {
  auto next_proposal = *upcoming_chain.at(block_height);

  LOG_TRACE << "DevcashController()::CreateNextProposal(): begin";
  LOG_INFO << "Upcoming #"+std::to_string(block_height)+" has "
    +std::to_string(next_proposal.vtx_.size())+" transactions.";

  if (!(block_height % 100) || !((block_height + 1) % 100)) {
    LOG_WARNING << "Processing @ final_chain_.size: (" << std::to_string(block_height) << ")";
  }

  // (nick@cs) we need to create a new block in the upcoming chain that accepts
  // incoming transactions from other validation workers before we copy the
  // previous back() of upcoming_chain into proposed_chain
  // In fact, this should happen before we get the next_proposal reference
  // in case a validator worker is currently adding a transaction to it.
  ProposedPtr upcoming_ptr = std::make_shared<ProposedBlock>(""
    , block_height+1, keys);
  upcoming_ptr->SetBlockState(next_proposal.GetBlockState());

  LOG_TRACE << "upcoming_chain.push_back()";
  upcoming_chain.push_back(upcoming_ptr);

  // (spmckenney) I'm not sure why proposal_pushed onto the proposed_chain_
  // and then immediately popped into new pointer...
  ProposedPtr proposal_ptr = std::make_shared<ProposedBlock>(next_proposal.vtx_,
                                                             next_proposal.GetValidationBlock(),
                                                             next_proposal.block_height_);

  LOG_INFO << "Proposal #" + std::to_string(block_height) + " has "
    + std::to_string(proposal_ptr->vtx_.size()) + " transactions.";

  // Validate and sign
  proposal_ptr->validate(keys);
  proposal_ptr->signBlock(keys.getNodeKey(context.get_current_node()),
                      context.kNODE_ADDRs[context.get_current_node()]);

  std::string proposal_str = proposal_ptr->ToJSON();

  LOG_TRACE << "proposed_chain.push_back()";
  proposed_chain.push_back(proposal_ptr);
  LOG_DEBUG << "Propose Block: "+proposal_str;
  std::vector<uint8_t> data(str2Bin(proposal_str));

  // Create message
  auto propose_msg = std::make_unique<DevcashMessage>("peers", PROPOSAL_BLOCK, data);
  LOG_TRACE << "DevcashController()::CreateNextProposal(): complete";
  return propose_msg;
}

void DevcashController::ValidatorCallback(DevcashMessageUniquePtr ptr) {
  LOG_DEBUG << "DevcashController::ValidatorCallback()";
  if (shutdown_) return;
  if (ptr->message_type == TRANSACTION_ANNOUNCEMENT) {
    DevcashMessage msg(*ptr.get());
    std::string tx_str = bin2Str(msg.data);
    LOG_DEBUG << "New transaction: "+tx_str;
    upcoming_chain_.back()->addTransaction(tx_str, keys_);
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
  //Make highest proposed block final
  //check if propose next
  //if so, send proposal with all pending valid txs
  DevcashMessage msg(*ptr.get());
  std::string final_block_str = bin2Str(msg.data);
  LOG_DEBUG << "Got final block: " + final_block_str;
  ProposedPtr highest_proposal = proposed_chain.back();
  DCBlock new_block(final_block_str, keys);
  new_block.SetBlockState(highest_proposal->GetBlockState());

  highest_proposal->GetValidationBlock().addValidation(new_block.GetValidationBlock());
  highest_proposal->finalize(GetHighestMerkleRoot(final_chain));

  // Did we send a message
  bool sent_message = false;

  if (highest_proposal->compare(new_block)) {
    // (nick@cs) the final block needs to have the validations in the order specified
    // by the remote node that finalized, so use final_block_str instead of highest_proposal
    FinalPtr top_block = FinalPtr(new FinalBlock(final_block_str,
                                                 highest_proposal->block_height_,
                                                 keys));
    /*FinalPtr top_block = FinalPtr(new FinalBlock(highest_proposal->vtx_,
      highest_proposal->GetValidationBlock(),
      highest_proposal->block_height_));*/
    top_block->copyHeaders(new_block);
    LOG_TRACE << "final_chain.push_back()";
    final_chain.push_back(top_block);

    if ((final_chain.size() % context.get_peer_count()) == context.get_current_node()) {
      LOG_INFO << "This node's turn to create proposal.";
      callback(std::move(CreateNextProposal(final_chain.size(),
                                  context,
                                  keys,
                                  proposed_chain,
                                  upcoming_chain)));
      // FIXME(spmckenney) accepting_valids_ = true;
      sent_message = true;
    }/* else if (waiting_ == 0) {
      millisecs ms =
        std::chrono::duration_cast<millisecs>(std::chrono::system_clock::now().time_since_epoch());
      waiting_ = ms.count();
    } else {
        millisecs ms =
        std::chrono::duration_cast<millisecs>(std::chrono::system_clock::now().time_since_epoch());
      if (ms.count() > static_cast<int>(waiting_ + kPROPOSAL_TIMEOUT)) {
        LOG_FATAL << "Proposal timed out at block #"
          +std::to_string(final_chain.size());
        // FIXME(spmckenney) handle LOG_FATAL and shutdown
        // StopAll();
        }

        } */
  } else {
    LOG_DEBUG << "Highest Proposal: "+highest_proposal->ToJSON();
    LOG_DEBUG << "Final block: "+new_block.ToJSON();
    LOG_FATAL << "Final block is inconsistent with chain.";
    // FIXME(spmckenney): handle LOG_FATAL and shutdown
    // StopAll();
  }
  return sent_message;
}
bool HandleProposalBlock(DevcashMessageUniquePtr ptr,
                         const DevcashContext& context,
                         const KeyRing& keys,
                         ProposedBlockchain& proposed_chain,
                         const ProposedBlockchain& upcoming_chain,
                         const FinalBlockchain& final_chain,
                         std::function<void(DevcashMessageUniquePtr)> callback) {
  //validate block
  //if valid, push VALID message
  bool sent_message = false;
  DevcashMessage msg(*ptr.get());
  std::string raw_str = bin2Str(msg.data);
  LOG_DEBUG << "Received block proposal: " + raw_str;
  unsigned int block_height = final_chain.size();
  ProposedPtr next_proposal = upcoming_chain.at(block_height);
  ProposedPtr new_proposal = std::make_shared<ProposedBlock>(raw_str,
                                                             final_chain.size(),
                                                             keys);
  new_proposal->SetBlockState(next_proposal->GetBlockState());
  if (new_proposal->validateBlock(keys)) {
    LOG_DEBUG << "Proposed block is valid.";
    LOG_TRACE << "proposed_chain.push_back()";
    proposed_chain.push_back(new_proposal);
    new_proposal->signBlock(keys.getNodeKey(context.get_current_node()),
                            context.kNODE_ADDRs[context.get_current_node()]);
    int proposer = (proposed_chain.size() - 1) % context.get_peer_count();
    raw_str = new_proposal->GetValidationBlock().ToJSON();
    LOG_DEBUG << "Validation: " + raw_str;
    std::vector<uint8_t> data(str2Bin(raw_str));

    std::string remote_uri = context.get_uri_from_index(proposer);
    auto valid = std::make_unique<DevcashMessage>(remote_uri,
                                                  VALID,
                                                  data);
    callback(std::move(valid));
    sent_message = true;
  } else {
    LOG_FATAL << "Proposed Block is invalid!\n"+
              new_proposal->ToJSON()+"\n----END of BLOCK-------\n";
    //StopAll();
  }
  LOG_DEBUG << "finished processing proposal";
  return sent_message;
}

bool HandleValidationBlock(DevcashMessageUniquePtr ptr,
                           const DevcashContext& context,
                           const KeyRing& keys,
                           const ProposedBlockchain& proposed_chain,
                           ProposedBlockchain& upcoming_chain,
                           FinalBlockchain& final_chain,
                           std::function<void(DevcashMessageUniquePtr)> callback) {
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
    LOG_TRACE << "final_chain.push_back()";
    final_chain.push_back(top_block);
    ProposedPtr upcoming = std::make_shared<ProposedBlock>(""
                           , upcoming_chain.size(), keys);
    upcoming->SetBlockState(proposed_chain.back()->GetBlockState());
    LOG_TRACE << "upcoming_chain.push_back()";
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
  return sent_message;
}

void DevcashController::ConsensusCallback(DevcashMessageUniquePtr ptr) {
  LOG_DEBUG << "DevcashController()::ConsensusCallback()";
  if (shutdown_) return;
  if (ptr->message_type == FINAL_BLOCK) {
    LOG_TRACE << "DevcashController()::ConsensusCallback(): FINAL_BLOCK begin";
    auto res = HandleFinalBlock(std::move(ptr),
                                context_,
                                keys_,
                                proposed_chain_,
                                upcoming_chain_,
                                final_chain_,
                                [this](DevcashMessageUniquePtr p) { this->server_.QueueMessage(std::move(p));});
    if (res) {
      accepting_valids_ = true;
    }
    LOG_TRACE << "DevcashController()::ConsensusCallback(): FINAL_BLOCK complete";
  } else if (ptr->message_type == PROPOSAL_BLOCK) {
    LOG_TRACE << "DevcashController()::ConsensusCallback(): PROPOSAL_BLOCK begin";
    auto res = HandleProposalBlock(std::move(ptr),
                                   context_,
                                   keys_,
                                   proposed_chain_,
                                   upcoming_chain_,
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
    LOG_TRACE << "DevcashController()::ConsensusCallback(): VALIDATION (" << accepting_valids_ << ") begin";
    std::lock_guard<std::mutex> lock(valid_lock_);
    LOG_TRACE << "DevcashController()::ConsensusCallback(): VALIDATION (" << accepting_valids_ << ") lock acquired";
    if (accepting_valids_) {
      auto res = HandleValidationBlock(std::move(ptr),
                                       context_,
                                       keys_,
                                       proposed_chain_,
                                       upcoming_chain_,
                                       final_chain_,
                                       [this](DevcashMessageUniquePtr p) { this->server_.QueueMessage(std::move(p));});
      accepting_valids_ = false;
    }
    LOG_TRACE << "DevcashController()::ConsensusCallback(): VALIDATION (" << accepting_valids_ << ") complete";
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

void DevcashController::SeedTransactions(std::string txs) {
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
}

bool DevcashController::PostAdvanceTransactions(const std::string& inputTxs) {
  CASH_TRY {
    int counter = 0;
    LOG_DEBUG << "Posting block height "+std::to_string(seeds_at_);
    if (upcoming_chain_.size()-1 < seeds_at_) {
      ProposedPtr next_proposal = upcoming_chain_.back();
      ProposedPtr upcoming_ptr = std::make_shared<ProposedBlock>(""
              , upcoming_chain_.size(), keys_);
          upcoming_ptr->SetBlockState(next_proposal->GetBlockState());
          LOG_TRACE << "upcoming_chain.push_back()";
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
    return true;
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "DevcashController.postTransactions");
  }
  return false;
}

bool DevcashController::PostTransactions() {
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
      return false;
    }
    return true;
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "DevcashController.PostTransactions");
  }
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
        server_.QueueMessage(CreateNextProposal(final_chain_.size(),
                                                context_,
                                                keys_,
                                                proposed_chain_,
                                                upcoming_chain_));
        accepting_valids_ = true;
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
  LOG_DEBUG << "DevcashController::PushValidator()";
  workers_->pushValidator(std::move(ptr));
}

void DevcashController::PushConsensus(DevcashMessageUniquePtr ptr) {
  LOG_DEBUG << "DevcashController::PushConsensus()";
  workers_->pushConsensus(std::move(ptr));
}

} //end namespace Devcash
