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

/**
 * Creates a proposal from the UnrecordedTransactionPool
 * @param[in] keys
 * @param[in, out] final_chain
 * @param[in, out] utx_pool
 * @param[in] context
 * @return
 */
std::vector<byte> CreateNextProposal(const KeyRing& keys,
                                           Blockchain& final_chain,
                                           UnrecordedTransactionPool& utx_pool,
                                           const DevcashContext& context);
/**
 * Registered with DevcashController and called when a eMessageType::FINAL_BLOCK
 * message arrives.
 * @param[in] ptr Pointer to incoming DevcashMessage
 * @param[in] context DevcashContext
 * @param[in] keys KeyRing keys
 * @param[in, out] final_chain The Blockchain to be updated with the incoming FINAL_BLOCK
 * @param[in, out] utx_pool
 * @param[in] callback Completion callback
 * @return
 */
bool HandleFinalBlock(DevcashMessageUniquePtr ptr,
                      const DevcashContext& context,
                      const KeyRing& keys,
                      Blockchain& final_chain,
                      UnrecordedTransactionPool& utx_pool,
                      std::function<void(DevcashMessageUniquePtr)> callback);

/**
 * Registered with DevcashController and called when a eMessageType::PROPOSAL_BLOCK message
 * arrives.
 * @param[in] ptr
 * @param[in] context
 * @param[in] keys
 * @param[in, out] final_chain
 * @param[in, out] tcm
 * @param[in] callback
 * @return
 */
bool HandleProposalBlock(DevcashMessageUniquePtr ptr,
                         const DevcashContext& context,
                         const KeyRing& keys,
                         Blockchain& final_chain,
                         TransactionCreationManager& tcm,
                         std::function<void(DevcashMessageUniquePtr)> callback);

/**
 *
 * @param ptr
 * @param context
 * @param final_chain
 * @param utx_pool
 * @param callback
 * @return
 */
bool HandleValidationBlock(DevcashMessageUniquePtr ptr,
                           const DevcashContext& context,
                           Blockchain& final_chain,
                           UnrecordedTransactionPool& utx_pool,
                           std::function<void(DevcashMessageUniquePtr)> callback);

/**
 *
 * @param ptr
 * @param final_chain
 * @param context
 * @param keys
 * @param remote_blocks
 * @return
 */
bool HandleBlocksSince(DevcashMessageUniquePtr ptr,
                       Blockchain& final_chain,
                       DevcashContext context,
                       const KeyRing& keys,
                       const UnrecordedTransactionPool&,
                       uint64_t& remote_blocks);

/**
 *
 * @param ptr
 * @param final_chain
 * @param context
 * @param keys
 * @param callback
 * @return
 */
bool HandleBlocksSinceRequest(DevcashMessageUniquePtr ptr,
                              Blockchain& final_chain,
                              const DevcashContext& context,
                              const KeyRing& keys,
                              std::function<void(DevcashMessageUniquePtr)> callback);
}  // namespace Devcash
