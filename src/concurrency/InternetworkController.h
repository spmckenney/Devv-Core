/*
 * ${Filename}
 *
 *  Created on: 6/22/18
 *      Author: mckenney
 */
#pragma once

#include <mutex>

#include "types/DevcashMessage.h"
#include "consensus/blockchain.h"
#include "consensus/UnrecordedTransactionPool.h"
#include "consensus/chainstate.h"

namespace Devcash {

class InternetworkController {
  /// typedef BlocksSinceCallback
  typedef std::function<bool(DevcashMessageUniquePtr ptr,
                             Blockchain &final_chain,
                             DevcashContext context,
                             const KeyRing &keys,
                             const UnrecordedTransactionPool &,
                             uint64_t &remote_blocks)> BlocksSinceCallback;
  /// typedef BlocksSinceRequestCallback
  typedef std::function<bool(DevcashMessageUniquePtr ptr,
                             Blockchain &final_chain,
                             const DevcashContext &context,
                             const KeyRing &keys,
                             std::function<void(DevcashMessageUniquePtr)> callback)> BlocksSinceRequestCallback;

 public:
  InternetworkController(const KeyRing& keys,
                      DevcashContext& context,
                      const ChainState& prior,
                      Blockchain& final_chain,
                      UnrecordedTransactionPool& utx_pool,
                      eAppMode mode);


  void registerOutgoingCallback(DevcashMessageCallback callback) {
    outgoing_callback_ = callback;
  }
  void messageCallback(DevcashMessageUniquePtr ptr);

 private:
  const KeyRing& keys_;
  DevcashContext& context_;
  Blockchain& final_chain_;
  UnrecordedTransactionPool& utx_pool_;
  eAppMode mode_;
  size_t remote_blocks_;

  std::mutex blockchain_mutex_;
  std::mutex utx_mutex_;
  /// A callback to send the analyzed DevcashMessage
  DevcashMessageCallback outgoing_callback_;

  BlocksSinceCallback blocks_since_cb_;
  BlocksSinceRequestCallback blocks_since_request_cb_;

};

} // namespace Devcash
