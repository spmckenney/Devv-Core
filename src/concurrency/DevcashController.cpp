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

namespace Devcash {

using namespace Devcash;

std::string DevcashController::getHighestMerkleRoot() {
  unsigned int block_height = final_chain_.size();
  std::string prevHash = "Genesis";
  if (block_height > 0) {
    prevHash = final_chain_.at(block_height-1)->hashMerkleRoot_;
  }
  return prevHash;
}

bool DevcashController::CreateNextProposal() {
  ProposedPtr next_proposal = upcoming_chain_.back();
  unsigned int block_height = final_chain_.size();
  LOG_INFO << "Upcoming #"+std::to_string(block_height)+" has "
      +std::to_string(next_proposal->vtx_.size())+" transactions.";
  if (block_height%context_.peer_count == context_.current_node_) {
    LOG_INFO << "This node's turn to create proposal.";
    ProposedPtr upcoming_ptr = std::make_shared<ProposedBlock>(""
        , upcoming_chain_.size(), keys_);
    upcoming_ptr->setBlockState(next_proposal->block_state_);
    upcoming_chain_.push_back(upcoming_ptr);
    ProposedPtr proposal_ptr = std::make_shared<ProposedBlock>(next_proposal->vtx_
        , next_proposal->vals_, next_proposal->block_height_);
    proposed_chain_.push_back(proposal_ptr);
    ProposedPtr proposal = proposed_chain_.back();
    LOG_INFO << "Proposal #"+std::to_string(block_height)+" has "
        +std::to_string(proposal->vtx_.size())+" transactions.";
    proposal->validate(keys_);
    proposal->signBlock(keys_.getNodeKey(context_.current_node_),
        context_.kNODE_ADDRs[context_.current_node_]);
    std::string proposal_str = proposal->ToJSON();
    LOG_DEBUG << "Propose Block: "+proposal_str;
    std::vector<uint8_t> data(str2Bin(proposal_str));
  auto propose_msg = std::make_unique<DevcashMessage>("peers", PROPOSAL_BLOCK, data);
    server_.QueueMessage(std::move(propose_msg));
    return true;
  } else if (waiting_ == 0) {
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>
      (std::chrono::system_clock::now().time_since_epoch());
    waiting_ = ms.count();
  } else {
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>
          (std::chrono::system_clock::now().time_since_epoch());
     if (ms.count() > waiting_+kPROPOSAL_TIMEOUT) {
       LOG_FATAL << "Proposal timed out at block #"
           +std::to_string(block_height);
       stopAll();
     }
  }
  return false;
}

void DevcashController::ValidatorCallback(DevcashMessageUniquePtr ptr) {
  LOG_DEBUG << "DevcashController::ValidatorCallback()";
  if (ptr->message_type == TRANSACTION_ANNOUNCEMENT) {
    DevcashMessage msg(*ptr.get());
    DCTransaction new_tx(bin2Str(msg.data));
    upcoming_chain_.back()->addTransaction(new_tx, keys_);
  } else {
    LOG_DEBUG << "Unexpected message @ validator, to consensus.\n";
    pushConsensus(std::move(ptr));
  }
}

void DevcashController::ConsensusCallback(DevcashMessageUniquePtr ptr) {
  LOG_DEBUG << "DevcashController()::ConsensusCallback()";
  if (ptr->message_type == FINAL_BLOCK) {
    //Make highest proposed block final
    //check if propose next
    //if so, send proposal with all pending valid txs
    DevcashMessage msg(*ptr.get());
    std::string final_block_str = bin2Str(msg.data);
    LOG_DEBUG << "Got final block: "+final_block_str;
    ProposedPtr highest_proposal = proposed_chain_.back();
    DCBlock new_block(final_block_str, keys_);
    new_block.setBlockState(highest_proposal->block_state_);

    highest_proposal->vals_.addValidation(new_block.vals_);
    highest_proposal->finalize(getHighestMerkleRoot());

    if (highest_proposal->compare(new_block)) {
      FinalPtr top_block = FinalPtr(new FinalBlock(highest_proposal->vtx_
          , highest_proposal->vals_, highest_proposal->block_height_));
      final_chain_.push_back(top_block);
      CreateNextProposal();
    } else {
      LOG_FATAL << "Final block is inconsistent with chain.";
      LOG_DEBUG << "Highest Proposal: "+highest_proposal->ToJSON();
      LOG_DEBUG << "Final block: "+new_block.ToJSON();
      stopAll();
    }
  } else if (ptr->message_type == PROPOSAL_BLOCK) {
    //validate block
    //if valid, push VALID message
    waiting_ = 0;
    DevcashMessage msg(*ptr.get());
    std::string raw_str = bin2Str(msg.data);
    LOG_DEBUG << "Received block proposal: "+raw_str;
    ProposedPtr upcoming_top = upcoming_chain_.back();
    ProposedPtr upcoming_ptr = std::make_shared<ProposedBlock>(""
        , upcoming_chain_.size(), keys_);
    upcoming_ptr->setBlockState(upcoming_top->block_state_);
    ProposedPtr new_proposal = std::make_shared<ProposedBlock>(raw_str, upcoming_chain_.size()
        , keys_);
    new_proposal->setBlockState(upcoming_top->block_state_);
    upcoming_chain_.push_back(upcoming_ptr);
    if (new_proposal->validateBlock(keys_)) {
      LOG_DEBUG << "Proposed block is valid.";
      proposed_chain_.push_back(new_proposal);
      new_proposal->signBlock(keys_.getNodeKey(context_.current_node_),
          context_.kNODE_ADDRs[context_.current_node_]);
      int proposer = proposed_chain_.size()%context_.peer_count;
      raw_str = new_proposal->vals_.ToJSON();
      LOG_DEBUG << "Validation: "+raw_str;
      std::vector<uint8_t> data(str2Bin(raw_str));
      auto valid = std::unique_ptr<DevcashMessage>(
        new DevcashMessage("RemoteURI-"+proposer, VALID,data));
      server_.QueueMessage(std::move(valid));
    } else {
      LOG_FATAL << "Proposed Block is invalid!\n"+
          new_proposal->ToJSON()+"\n----END of BLOCK-------\n";
      stopAll();
    }
    LOG_DEBUG << "finished processing proposal";
  } else if (ptr->message_type == TRANSACTION_ANNOUNCEMENT) {
    LOG_DEBUG << "Unexpected message @ consensus, to validator.\n";
    pushValidator(std::move(ptr));
  } else if (ptr->message_type == VALID) {
    //increment validation count
    //if count >= validationPercent, finalize block
    DevcashMessage msg(*ptr.get());
    std::string rawVal = bin2Str(msg.data);
    LOG_DEBUG << "Received block validation: "+rawVal;
    unsigned int block_height = final_chain_.size();
    if (block_height%context_.peer_count != context_.current_node_) {
      LOG_WARNING << "Got a VALID message, but this node did not propose!";
      return;
    }
    DCValidationBlock validation(rawVal);
    ProposedBlock highest_proposal = *proposed_chain_.back().get();
    highest_proposal.vals_.addValidation(validation);
    if (highest_proposal.vals_.GetValidationCount() > 1) {
      highest_proposal.finalize(getHighestMerkleRoot());
      FinalPtr top_block =std::make_shared<FinalBlock>(highest_proposal
          , highest_proposal.block_height_);
      final_chain_.push_back(top_block);
      ProposedPtr upcoming = std::make_shared<ProposedBlock>(""
          , upcoming_chain_.size(), keys_);
      upcoming->setBlockState(proposed_chain_.back()->block_state_);
      upcoming_chain_.push_back(upcoming);
      std::string final_str = top_block->ToJSON();
      LOG_DEBUG << "Final block: "+final_str;
      std::vector<uint8_t> data(str2Bin(final_str));
      auto finalBlock = std::make_unique<DevcashMessage>("peers", FINAL_BLOCK, data);
      server_.QueueMessage(std::move(finalBlock));
    } else {
      unsigned int vals = proposed_chain_.back()->vals_.GetValidationCount();
      LOG_INFO << "Block proposal validated "+std::to_string(vals)+" times.\n";
    }
  } else if (ptr->message_type == REQUEST_BLOCK) {
    //provide blocks since requested height
    LOG_DEBUG << "Got REQUEST BLOCK\n";
  } else {
    LOG_DEBUG << "Unexpected message @ consensus, ignore.\n";
  }
}

void DevcashController::ValidatorToyCallback(DevcashMessageUniquePtr ptr) {
  LOG_DEBUG << "DevcashController::ValidatorToyCallback()";
  if (validator_flipper) {
    pushConsensus(std::move(ptr));
  }
  validator_flipper = !validator_flipper;
}

void DevcashController::ConsensusToyCallback(DevcashMessageUniquePtr ptr) {
  LOG_DEBUG << "DevcashController()::ConsensusToyCallback()";
  if (consensus_flipper) {
    server_.QueueMessage(std::move(ptr));
  }
  consensus_flipper = !consensus_flipper;
}

DevcashController::DevcashController(io::TransactionServer& server,
                                     io::TransactionClient& client,
                                     int validatorCount,
                                     int consensusWorkerCount,
                                     KeyRing& keys,
                                     DevcashContext& context)
  : server_(server)
  , client_(client)
  , validator_count_(validatorCount)
  , consensus_count_(consensusWorkerCount)
  , keys_(keys)
  , context_(context)
  , seeds_at_(0)
  , workers_(new DevcashControllerWorker(this, validator_count_, consensus_count_))
{
  keys_.initKeys();
  LOG_INFO << "Crypto Keys initialized.";

  ProposedPtr upcoming = std::make_shared<ProposedBlock>();
  upcoming_chain_.push_back(upcoming);
  LOG_INFO << "Upcoming chain created";
}

void DevcashController::seedTransactions(std::string txs) {
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
            stopAll();
          }
          std::string txSubstr(toParse.substr(dex, eDex-dex+2));
          seeds_.push_back(txSubstr);
          break;
        } else {
          std::string txSubstr(toParse.substr(dex, eDex-dex+2));
          seeds_.push_back(txSubstr);
          dex = toParse.find("[", eDex);
        }
      }
      LOG_INFO << "Seeded input for "+std::to_string(seeds_.size())+" blocks.";
      postTransactions();
    } else {
      LOG_FATAL << "Input has wrong syntax!";
      stopAll();
    }
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "DevcashController.seedTransactions");
  }
}

bool DevcashController::postTransactions() {
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
      DCTransaction new_tx(oneTx);
      upcoming->addTransaction(new_tx, keys_);
      counter++;
      while (someTxs.at(eDex+1) != ']' && eDex < someTxs.size()-2) {
        dex = someTxs.find("{", eDex);
        eDex = someTxs.find(kSIG_TAG, dex);
        eDex = someTxs.find("}", eDex);
        oneTx = someTxs.substr(dex, eDex-dex);
        DCTransaction next_tx(oneTx);
        upcoming->addTransaction(next_tx, keys_);
        counter++;
      }
      LOG_DEBUG << std::to_string(counter)+" transactions posted upcoming.";
    } else { //all input processed by the chain
      return false;
    }
    return true;
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "DevcashController.postTransactions");
  }
  return false;
}

void DevcashController::StartToy(unsigned int node_index) {
  workers_->start();

  LOG_DEBUG << "READY? StartToy()";
  client_.AttachCallback([this](DevcashMessageUniquePtr ptr) {
      if (ptr->message_type == TRANSACTION_ANNOUNCEMENT) {
          pushValidator(std::move(ptr));
        } else {
          pushConsensus(std::move(ptr));
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
    sleep(60);
  }
}

bool DevcashController::start() {
  workers_->start();

  client_.AttachCallback([this](DevcashMessageUniquePtr ptr) {
    if (ptr->message_type == TRANSACTION_ANNOUNCEMENT) {
        pushValidator(std::move(ptr));
      } else {
        pushConsensus(std::move(ptr));
      }
  });

  CreateNextProposal();

  //run as node 0
  /*std::string valid("\"sum\":{\"02514038DA1905561BF9043269B8515C1E7C4E79B011291B4CBED5B18DAECB71E4\":[(0,10)],\"035C0841F8F62271F3058F37B32193360322BBF0C4E85E00F07BCB10492E91A2BD\":[(0,10)],\"7242DC4F1D89513CBA236C895B117BC7D0ABD6DC8336E202D93FB266E582C79624\":[(0,-20)]},\"vals\":[\"04B14F28DA8C0389BC385BA3865DB3FC7FAFA8FA4715C0ADAADAC52F2EB3E7FDCD695B439F9ACDCC90E55C1F9C48D7EB5B3BFD6C64EC89B1A6108F4B1B01A3FCA4\":\"30440220661C2A266DD00E1850172AFC11BBB9772FAA687B00D21A0FDB4EC6FB1EF14516022000EEF25E079C694304BD33BF1DAA8A3C58EF612BF95DCB4BC7F1754707B60D84\",\"04158913328E4469124B33A5D421665A36891B7BCB8183A22CC3D78239A89073FEB7432E6477663CDE2E032A56687617800B97FC0EBD9F3AC30F683B6C4A89D1D8\":\"304402203631D6DE6614642A2356E04A5A80BDEF8BEB1E4D5411BE77088036E04883C21A022071B0D47AA66F5A243861068D5597256D2D378E405FFB2BC479A62632044A2BE4\"]");
  std::vector<uint8_t> data(str2Bin(valid));
  auto v = DevcashMessageUniquePtr(
            new DevcashMessage("peers", VALID, data));
  ConsensusCallback(std::move(v));
  LOG_DEBUG << "Valid test complete.";*/
  //end run as node 0

  //run as node 1/2
  /*std::string validBlock("{\"prev\":\"\",\"merkle\":\"\",\"bytes\":0,\"time\":0,\"txlen\":0,\"sumlen\":0,\"vlen\":0,\"txs\":[{\"oper\":0,\"xfer\":[{\"addr\":\"7242DC4F1D89513CBA236C895B117BC7D0ABD6DC8336E202D93FB266E582C79624\",\"type\":0,\"amount\":-20},{\"addr\":\"02514038DA1905561BF9043269B8515C1E7C4E79B011291B4CBED5B18DAECB71E4\",\"type\":0,\"amount\":10},{\"addr\":\"035C0841F8F62271F3058F37B32193360322BBF0C4E85E00F07BCB10492E91A2BD\",\"type\":0,\"amount\":10}],\"nonce\":1521706664,\"sig\":\"304402206C7179523F204125D3C63F9817460B211784B071B608625933E646F46CD5BAD10220088BBA2D10EE2EBA3A1D5BB40A323D5CBD2CD7946A315104F4FAA2A3B582DFF0\"}],\"sum\":{\"02514038DA1905561BF9043269B8515C1E7C4E79B011291B4CBED5B18DAECB71E4\":[(0,10)],\"035C0841F8F62271F3058F37B32193360322BBF0C4E85E00F07BCB10492E91A2BD\":[(0,10)],\"7242DC4F1D89513CBA236C895B117BC7D0ABD6DC8336E202D93FB266E582C79624\":[(0,-20)]},\"vals\":[\"04158913328E4469124B33A5D421665A36891B7BCB8183A22CC3D78239A89073FEB7432E6477663CDE2E032A56687617800B97FC0EBD9F3AC30F683B6C4A89D1D8\":\"304402203631D6DE6614642A2356E04A5A80BDEF8BEB1E4D5411BE77088036E04883C21A022071B0D47AA66F5A243861068D5597256D2D378E405FFB2BC479A62632044A2BE4\"]}");
  std::vector<uint8_t> data(str2Bin(validBlock));
  auto valid = DevcashMessageUniquePtr(
          new DevcashMessage("peers", PROPOSAL_BLOCK, data));
  ConsensusCallback(std::move(valid));
  LOG_DEBUG << "Proposal test complete.";

  postTransactions();

  std::string final_block("{\"prev\":\"Genesis\",\"merkle\":\"8AB95FBBF98096B40FD613022D89641ABB21ECD285CFFA521E22F3955644E0EC\",\"bytes\":1796,\"time\":1522385072962,\"txlen\":485,\"sumlen\":236,\"vlen\":1075,\"txs\":[{\"oper\":0,\"xfer\":[{\"addr\":\"7242DC4F1D89513CBA236C895B117BC7D0ABD6DC8336E202D93FB266E582C79624\",\"type\":0,\"amount\":-20},{\"addr\":\"02514038DA1905561BF9043269B8515C1E7C4E79B011291B4CBED5B18DAECB71E4\",\"type\":0,\"amount\":10},{\"addr\":\"035C0841F8F62271F3058F37B32193360322BBF0C4E85E00F07BCB10492E91A2BD\",\"type\":0,\"amount\":10}],\"nonce\":1521706664,\"sig\":\"304402206C7179523F204125D3C63F9817460B211784B071B608625933E646F46CD5BAD10220088BBA2D10EE2EBA3A1D5BB40A323D5CBD2CD7946A315104F4FAA2A3B582DFF0\"}],\"sum\":{\"02514038DA1905561BF9043269B8515C1E7C4E79B011291B4CBED5B18DAECB71E4\":[(0,10)],\"035C0841F8F62271F3058F37B32193360322BBF0C4E85E00F07BCB10492E91A2BD\":[(0,10)],\"7242DC4F1D89513CBA236C895B117BC7D0ABD6DC8336E202D93FB266E582C79624\":[(0,-20)]},\"vals\":[\"04B14F28DA8C0389BC385BA3865DB3FC7FAFA8FA4715C0ADAADAC52F2EB3E7FDCD695B439F9ACDCC90E55C1F9C48D7EB5B3BFD6C64EC89B1A6108F4B1B01A3FCA4\":\"30440220661C2A266DD00E1850172AFC11BBB9772FAA687B00D21A0FDB4EC6FB1EF14516022000EEF25E079C694304BD33BF1DAA8A3C58EF612BF95DCB4BC7F1754707B60\",\"04158913328E4469124B33A5D421665A36891B7BCB8183A22CC3D78239A89073FEB7432E6477663CDE2E032A56687617800B97FC0EBD9F3AC30F683B6C4A89D\":\"304402203631D6DE6614642A2356E04A5A80BDEF8BEB1E4D5411BE77088036E04883C21A022071B0D47AA66F5A243861068D5597256D2D378E405FFB2BC479A62632044A2B\",\"04158913328E4469124B33A5D421665A36891B7BCB8183A22CC3D78239A89073FEB7432E6477663CDE2E032A56687617800B97FC0EBD9F3AC30F683B6C4A89D1D8\":\"3046022100D1D889E2EC239A5512CBA5952DE72E2C53B4E04F5522B59E87EE1F3FC5C40802022100B49AD415EF15EB8035D76BACC3F755AB323274C2C2A0FFCB9FCC369531D2B363\"]}");
  std::vector<uint8_t> final_data(str2Bin(final_block));
  auto v = DevcashMessageUniquePtr(
           new DevcashMessage("peers", FINAL_BLOCK, final_data));
  ConsensusCallback(std::move(v));
  LOG_DEBUG << "Final block test complete.";*/
  //end run as node 1/2


  //Loop for long runs
  /* bool transactions_to_post = postTransactions();
   * auto ms = kMAIN_WAIT_INTERVAL;
  while (true) {
    LOG_DEBUG << "Sleeping for " << ms;
    boost::this_thread::sleep_for(boost::chrono::milliseconds(ms));
    if (transactions_to_post)
      transactions_to_post = postTransactions();
    if (final_chain_.size() >= seeds_.size() ) {
      return true; //EXIT_SUCCESS
    }
    //EXIT_FAILURE
    if (shutdown) return false;
   }
   *
   */

  return true;
}

void DevcashController::stopAll() {
  shutdown = true;
  workers_->stopAll();
}

void DevcashController::pushValidator(DevcashMessageUniquePtr ptr) {
  LOG_DEBUG << "DevcashController::pushValidator()";
  workers_->pushValidator(std::move(ptr));
}

void DevcashController::pushConsensus(DevcashMessageUniquePtr ptr) {
  LOG_DEBUG << "DevcashController::pushConsensus()";
  workers_->pushConsensus(std::move(ptr));
}

} //end namespace Devcash
