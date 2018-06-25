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
#include "consensus/tier2_message_handlers.h"

typedef std::chrono::milliseconds millisecs;

namespace fs = boost::filesystem;

namespace Devcash {

#define DEBUG_TRANSACTION_INDEX \
  (processed + (context_.get_current_node()+1)*11000000)

ValidatorController::ValidatorController(
    const KeyRing& keys,
    DevcashContext& context,
    const ChainState&,
    Blockchain& final_chain,
    UnrecordedTransactionPool& utx_pool,
    eAppMode mode)
    : keys_(keys)
    , context_(context)
    , final_chain_(final_chain)
    , utx_pool_(utx_pool)
    , mode_(mode)
    , tx_announcement_cb_(CreateNextProposal)
{
  LOG_DEBUG << "ValidatorController created - mode " << mode_;
}

ValidatorController::~ValidatorController() {
  LOG_DEBUG << "~ValidatorController()";
}

void ValidatorController::validatorCallback(DevcashMessageUniquePtr ptr) {
  LOG_DEBUG << "ValidatorController::validatorCallback()";
  std::lock_guard<std::mutex> guard(mutex_);
  if (ptr == nullptr) {
    throw DevcashMessageError("validatorCallback(): ptr == nullptr, ignoring");
  }
  LogDevcashMessageSummary(*ptr, "validatorCallback");

  LOG_DEBUG << "ValidatorController::validatorCallback()";
  MTR_SCOPE_FUNC();
  if (ptr->message_type == TRANSACTION_ANNOUNCEMENT) {
    DevcashMessage msg(*ptr.get());
    utx_pool_.AddTransactions(msg.data, keys_);
    if (context_.get_current_node() % context_.get_peer_count() == 0) {
      size_t block_height = final_chain_.size();
      if (block_height == 0) {
        LOG_INFO << "(spm): CreateNextProposal, utx_pool.HasProposal(): " << utx_pool_.HasProposal();
        if (!utx_pool_.HasProposal()) {
          outgoing_callback_(std::move(
              tx_announcement_cb_(keys_, final_chain_, utx_pool_, context_)));
        }
      }
    }
  } else {
    throw DevcashMessageError("Wrong message type arrived: " + std::to_string(ptr->message_type));
  }
}

/*
void ValidatorController::Start() {
  MTR_SCOPE_FUNC();
  try {

    std::vector<std::vector<byte>> transactions;
    size_t processed = 0;

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

      // Should we announce a transaction?
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

      // Should we shutdown?
      if (fs::exists(stop_file_)) {
        LOG_INFO << "shutdown file detected. Stopping DevCash...";
        StopAll();
      }
    }

  } catch (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "ValidatorController.Start()");
    StopAll();
  }
}
*/
} //end namespace Devcash
