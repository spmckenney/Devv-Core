/**
 * ${Filename}
 *
 *  Created on: 6/22/18
 *      Author: mckenney
 */
#include "ConsensusController.h"
#include "consensus/tier2_message_handlers.h"

namespace Devcash {

ConsensusController::ConsensusController(const KeyRing &keys,
                                         DevcashContext &context,
                                         const ChainState &prior,
                                         Blockchain &final_chain,
                                         UnrecordedTransactionPool &utx_pool,
                                         eAppMode mode)
    : keys_(keys)
    , context_(context)
    , final_chain_(final_chain)
    , utx_pool_(utx_pool)
    , mode_(mode)
, final_block_cb_(HandleFinalBlock)
, proposal_block_cb_(HandleProposalBlock)
, validation_block_cb_(HandleValidationBlock)
{
}

void ConsensusController::consensusCallback(DevcashMessageUniquePtr ptr) {
  std::lock_guard<std::mutex> guard(mutex_);
  MTR_SCOPE_FUNC();
  try {
    switch (ptr->message_type) {
      case eMessageType::FINAL_BLOCK:LOG_DEBUG << "ValidatorController()::consensusCallback(): FINAL_BLOCK";
        final_block_cb_(std::move(ptr),
                                          context_,
                                          keys_,
                                          final_chain_,
                                          utx_pool_,
                                          [this](DevcashMessageUniquePtr p) {
                                            this->outgoing_callback_(std::move(p));
                                          });
        break;

      case eMessageType::PROPOSAL_BLOCK:LOG_DEBUG << "ValidatorController()::consensusCallback(): PROPOSAL_BLOCK";
        utx_pool_.get_transaction_creation_manager().set_keys(&keys_);
        proposal_block_cb_(std::move(ptr),
                                             context_,
                                             keys_,
                                             final_chain_,
                                             utx_pool_.get_transaction_creation_manager(),
                                             [this](DevcashMessageUniquePtr p) { this->outgoing_callback_(std::move(p)); });
        break;

      /*
        case eMessageType::TRANSACTION_ANNOUNCEMENT:
        LOG_DEBUG << "ValidatorController()::consensusCallback(): TRANSACTION_ANNOUNCEMENT";
        LOG_WARNING << "Unexpected message @ consensus, to validator";
        pushValidator(std::move(ptr));
        break;
*/
      case eMessageType::VALID:LOG_DEBUG << "ValidatorController()::consensusCallback(): VALIDATION";
        validation_block_cb_(std::move(ptr),
                                               context_,
                                               final_chain_,
                                               utx_pool_,
                                               [this](DevcashMessageUniquePtr p) {
                                                 this->outgoing_callback_(std::move(p));
                                               });
        break;
      /*
      case eMessageType::REQUEST_BLOCK:LOG_DEBUG << "Unexpected message @ consensus, to shard comms.";
        pushShardComms(std::move(ptr));
        break;

      case eMessageType::GET_BLOCKS_SINCE:LOG_DEBUG << "Unexpected message @ consensus, to shard comms.\n";
        pushShardComms(std::move(ptr));
        break;

      case eMessageType::BLOCKS_SINCE:LOG_DEBUG << "Unexpected message @ consensus, to shard comms.\n";
        pushShardComms(std::move(ptr));
        break;
      */
      default:
        throw DevcashMessageError("consensusCallback(): Unexpected message type:"
                                      + std::to_string(ptr->message_type));
    }
  } catch (const std::exception &e) {
    LOG_FATAL << FormatException(&e, "ValidatorController.consensusCallback()");
    throw e;
  }
}

} // namespace Devcash
