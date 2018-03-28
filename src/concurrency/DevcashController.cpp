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

void DevcashController::ValidatorCallback(DevcashMessageUniquePtr ptr) {
  LOG_DEBUG << "Validator callback triggered!";
  if (validator_flipper) {
    pushConsensus(std::move(ptr));
  }
  validator_flipper = !validator_flipper;
}

void DevcashController::ConsensusCallback(DevcashMessageUniquePtr ptr) {
  std::thread::id id = std::this_thread::get_id();
  LOG_INFO << "Consensus Callback "<<id<<" triggered!";
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
                                     DevcashContext& context,
                                     ProposedBlock& nextBlock,
                                     ProposedBlock& futureBlock)
  : server_(server)
  , client_(client)
  , validator_count_(validatorCount)
  , consensus_count_(consensusWorkerCount)
  , keys_(keys)
  , context_(context)
  , next_block_(nextBlock)
  , future_block_(futureBlock)
  , seeds_at_(0)
  , workers_(new DevcashControllerWorker(this, validator_count_, consensus_count_))
{
}

void DevcashController::rotateBlocks() {
  next_block_.setChainState(future_block_.getChainState());
  next_block_.setBlockHeight(final_chain_.size()+1);
  future_block_.SetNull();
  future_block_.setBlockHeight(final_chain_.size()+2);
  future_block_.setChainState(next_block_.getChainState());
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
          if (eDex == std::string::npos) LOG_WARNING << "Invalid input.";
          std::string txSubstr(toParse.substr(dex, eDex-dex+2));
          seeds_.push_back(txSubstr);
          break;
        } else {
          std::string txSubstr(toParse.substr(dex, eDex-dex+2));
          seeds_.push_back(txSubstr);
          dex = toParse.find("[", eDex);
        }
        postTransactions();
      }
    } else {
      LOG_WARNING << "Input has wrong syntax!";
    }
    LOG_INFO << "Seeded "+std::to_string(counter)+" input blocks.";
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "DevcashController.seedTransactions");
  }
}

void DevcashController::postTransactions() {
  CASH_TRY {
    unsigned int blockHeight = final_chain_.size();
    if (seeds_at_ > blockHeight) return;
    LOG_DEBUG << "Post input for height "+std::to_string(blockHeight)+"\n";
    LOG_DEBUG << "Seeds size is "+std::to_string(seeds_.size())+"\n";
    if (seeds_.size() > blockHeight) {
      std::string someTxs = seeds_.at(seeds_at_);
      seeds_at_++;
      size_t dex = someTxs.find("\""+kOPER_TAG+"\":", 0);
      size_t eDex = someTxs.find(kSIG_TAG, dex);
      eDex = someTxs.find("}", eDex);
      std::string oneTx = someTxs.substr(dex-1, eDex-dex+2);
      std::vector<uint8_t> data(str2Bin((new DCTransaction(oneTx))->ToJSON()));
      auto announce = DevcashMessageUniquePtr(
        new DevcashMessage("self", TRANSACTION_ANNOUNCEMENT,data));
      pushValidator(std::move(announce));
      while (someTxs.at(eDex+1) != ']' && eDex < someTxs.size()-2) {
        dex = someTxs.find("{", eDex);
        eDex = someTxs.find(kSIG_TAG, dex);
        eDex = someTxs.find("}", eDex);
        oneTx = someTxs.substr(dex, eDex-dex);
        std::vector<uint8_t> moreData(
            str2Bin((new DCTransaction(oneTx))->ToJSON()));
        announce = DevcashMessageUniquePtr(
          new DevcashMessage("self", TRANSACTION_ANNOUNCEMENT, moreData));
        pushValidator(std::move(announce));
      }
    }
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "DevcashController.postTransactions");
  }
}

void DevcashController::StartToy(unsigned int node_index) {
  workers_->start();

  client_.AttachCallback([this](DevcashMessageUniquePtr ptr) {
      if (ptr->message_type == TRANSACTION_ANNOUNCEMENT) {
          pushValidator(std::move(ptr));
        } else {
          pushConsensus(std::move(ptr));
        }
    });

  server_.StartServer();
  client_.StartClient();


  sleep(10);

  for (;;) {
    std::string uri = "RemoteURI-" + node_index;
    //TODO: send a message like this for each remote
    std::vector<uint8_t> data(100);
    auto startMsg = std::make_unique<DevcashMessage>(uri,
                                                     TRANSACTION_ANNOUNCEMENT,
                                                     data);
    server_.QueueMessage(std::move(startMsg));
    sleep(3);
  }
}

void DevcashController::start() {
  workers_->start();
  postTransactions();

  client_.AttachCallback([this](DevcashMessageUniquePtr ptr) {
      if (ptr->message_type == TRANSACTION_ANNOUNCEMENT) {
          pushValidator(std::move(ptr));
        } else {
          pushConsensus(std::move(ptr));
        }
    });

  if (0 == context_.current_node_) {
    next_block_.signBlock(keys_.getNodeKey(context_.current_node_));
    std::vector<uint8_t> data(str2Bin(next_block_.ToJSON()));
    auto proposal = DevcashMessageUniquePtr(
        new DevcashMessage("peers", PROPOSAL_BLOCK,data));
    server_.QueueMessage(std::move(proposal));
  }
}

void DevcashController::stopAll() {
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
