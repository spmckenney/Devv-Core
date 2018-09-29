/*
 * ValidatorController.h controls worker threads for the Devcash protocol.
 *
 *  Created on: Mar 21, 2018
 *      Author: Nick Williams
 */
#ifndef CONCURRENCY_DEVCASHCONTROLLER_H_
#define CONCURRENCY_DEVCASHCONTROLLER_H_

#include <condition_variable>
#include <mutex>

#include "consensus/KeyRing.h"
#include "consensus/UnrecordedTransactionPool.h"
#include "consensus/blockchain.h"
#include "consensus/chainstate.h"
#include "io/message_service.h"

namespace Devcash {

class ValidatorController {
  typedef std::function<std::vector<byte>(const KeyRing &keys,
                                                Blockchain &final_chain,
                                                UnrecordedTransactionPool &utx_pool,
                                                const DevcashContext &context)> TransactionAnnouncementCallback;

 public:
  ValidatorController(const KeyRing& keys,
                      DevcashContext& context,
                      const ChainState&,
                      Blockchain& final_chain,
                      UnrecordedTransactionPool& utx_pool,
                      eAppMode mode);

  ~ValidatorController();

  void registerOutgoingCallback(DevcashMessageCallback callback) {
    outgoing_callback_ = callback;
  }
  /**
   * Process a validator worker message.
   */
  void validatorCallback(DevcashMessageUniquePtr ptr);

 private:
  const KeyRing& keys_;
  DevcashContext& context_;
  Blockchain& final_chain_;
  UnrecordedTransactionPool& utx_pool_;
  eAppMode mode_;

  mutable std::mutex mutex_;
  //uint64_t remote_blocks_ = 0;

  /// A callback to send the analyzed DevcashMessage
  DevcashMessageCallback outgoing_callback_;

  TransactionAnnouncementCallback tx_announcement_cb_;
};

} /* namespace Devcash */

#endif /* CONCURRENCY_DEVCASHCONTROLLER_H_ */
