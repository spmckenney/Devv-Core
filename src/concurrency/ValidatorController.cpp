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
    size_t block_height = final_chain_.size();
    if (block_height%context_.get_peer_count() == context_.get_current_node()%context_.get_peer_count()) {
      LOG_INFO << "Received txs: CreateNextProposal? utx_pool.HasProposal(): " << utx_pool_.HasProposal();
      if (!utx_pool_.HasProposal()) {
        std::vector<byte> proposal = CreateNextProposal(keys,final_chain,utx_pool,context);
        if (!ProposedBlock::isNullProposal(proposal)) {
          // Create message
           auto propose_msg = std::make_unique<DevcashMessage>(context.get_shard_uri()
	                                                        , PROPOSAL_BLOCK
	                                                        , proposal
	                                                        , DEBUG_PROPOSAL_INDEX);
          // FIXME (spm): define index value somewhere
          LogDevcashMessageSummary(*propose_msg, "CreateNextProposal");
          outgoing_callback_(std::move(propose_msg));
        }
      }
    } else {
      LOG_INFO << "NOT PROPOSING! (" << block_height%context_.get_peer_count() << ")" <<
            " (" << context_.get_current_node()%context_.get_peer_count() << ")";
    }
  } else {
    throw DevcashMessageError("Wrong message type arrived: " + std::to_string(ptr->message_type));
  }
}

} //end namespace Devcash
