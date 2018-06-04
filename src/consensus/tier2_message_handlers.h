/**
 * Copyright (C) 2018 Devvio, Inc - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license, which unfortunately won't be
 * written for another century.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please visit http://www.devv.io
 *
 *   Created on: Jun 1, 2018
 *       Author: Nick Williams <nick@cloudsolar.co>
 *       Author: Shawn McKenney <shawn.mckenney@emmion.com>
**/
#include "types/DevcashMessage.h"
#include "common/devcash_context.h"
#include "common/logger.h"
#include "common/util.h"
#include "consensus/blockchain.h"
#include "consensus/UnrecordedTransactionPool.h"

namespace Devcash {

#define DEBUG_PROPOSAL_INDEX                                    \
  ((block_height+1) + (context.get_current_node()+1)*1000000)

#define DEBUG_TRANSACTION_INDEX                             \
  (processed + (context_.get_current_node()+1)*11000000)

/**
 *
 */
DevcashMessageUniquePtr CreateNextProposal(const KeyRing& keys,
                                           Blockchain& final_chain,
                                           UnrecordedTransactionPool& utx_pool,
                                           const DevcashContext& context);
/**
 *
 */
bool HandleFinalBlock(DevcashMessageUniquePtr ptr,
                      const DevcashContext& context,
                      const KeyRing& keys,
                      Blockchain& final_chain,
                      UnrecordedTransactionPool& utx_pool,
                      std::function<void(DevcashMessageUniquePtr)> callback);

/**
 *
 */
bool HandleProposalBlock(DevcashMessageUniquePtr ptr,
                         const DevcashContext& context,
                         const KeyRing& keys,
                         Blockchain& final_chain,
                         TransactionCreationManager& tcm,
                         std::function<void(DevcashMessageUniquePtr)> callback);

/**
 *
 */
bool HandleValidationBlock(DevcashMessageUniquePtr ptr,
                           const DevcashContext& context,
                           Blockchain& final_chain,
                           UnrecordedTransactionPool& utx_pool,
                           std::function<void(DevcashMessageUniquePtr)> callback);

/**
 *
 */
bool HandleBlocksSince(DevcashMessageUniquePtr ptr,
                       Blockchain& final_chain,
                       DevcashContext context,
                       const KeyRing& keys,
                       const UnrecordedTransactionPool&,
                       uint64_t& remote_blocks);

/**
 *
 */
bool HandleBlocksSinceRequest(DevcashMessageUniquePtr ptr,
                              Blockchain& final_chain,
                              const DevcashContext& context,
                              const KeyRing& keys,
                              std::function<void(DevcashMessageUniquePtr)> callback);
}  // namespace Devcash
