/*
 * ConsensusController.h
 *
 *  Created on: 6/22/18
 *      Authors: Shawn McKenney
 *               Nick Williams
 */
#pragma once

#include "types/DevcashMessage.h"
#include "consensus/UnrecordedTransactionPool.h"
#include "consensus/blockchain.h"
#include "consensus/chainstate.h"

namespace Devcash {

class ConsensusController {
typedef std::function<bool(DevcashMessageUniquePtr ptr,
                           const DevcashContext &context,
                           const KeyRing &keys,
                           Blockchain &final_chain,
                           UnrecordedTransactionPool &utx_pool,
                           std::function<void(DevcashMessageUniquePtr)> callback)> FinalBlockCallback;

typedef std::function<bool(DevcashMessageUniquePtr ptr,
                           const DevcashContext &context,
                           const KeyRing &keys,
                           Blockchain &final_chain,
                           TransactionCreationManager &tcm,
                           std::function<void(DevcashMessageUniquePtr)> callback)> ProposalBlockCallback;

typedef std::function<bool(DevcashMessageUniquePtr ptr,
                           const DevcashContext &context,
                           Blockchain &final_chain,
                           UnrecordedTransactionPool &utx_pool,
                           std::function<void(DevcashMessageUniquePtr)> callback)> ValidationBlockCallback;
 public:
  ConsensusController(const KeyRing& keys,
                      DevcashContext& context,
                      const ChainState& prior,
                      Blockchain& final_chain,
                      UnrecordedTransactionPool& utx_pool,
                      eAppMode mode);

  void registerOutgoingCallback(DevcashMessageCallback callback) {
    outgoing_callback_ = callback;
  }
  /**
   * Process a consensus worker message.
   */
  void consensusCallback(DevcashMessageUniquePtr ptr);

 private:
  const KeyRing& keys_;
  DevcashContext& context_;
  Blockchain& final_chain_;
  UnrecordedTransactionPool& utx_pool_;
  eAppMode mode_;

  mutable std::mutex mutex_;

  /// A callback to send the analyzed DevcashMessage
  DevcashMessageCallback outgoing_callback_;

  FinalBlockCallback final_block_cb_;
  ProposalBlockCallback proposal_block_cb_;
  ValidationBlockCallback validation_block_cb_;

};

} // namespace Devcash
