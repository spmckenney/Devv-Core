/*
 * DevcashWorker.h
 *
 *  Created on: Mar 26, 2018
 *      Author: Silver
 */

#ifndef CONCURRENCY_DEVCASHWORKER_H_
#define CONCURRENCY_DEVCASHWORKER_H_
#include <functional>
#include <thread>

#include <boost/thread/thread.hpp>
#include <boost/asio/io_service.hpp>

#include "types/DevcashMessage.h"
#include "common/logger.h"
#include "common/util.h"
#include "DevcashController.h"
#include "DevcashMPMCQueue.h"

namespace Devcash {

class DevcashControllerWorker {
 public:

  /* Constructors/Destructors */
  DevcashControllerWorker(DevcashController* control,
                    const int validators, const int consensus);

  virtual ~DevcashControllerWorker() {
    continue_ = false;
    validators_.ClearBlockers();
    consensus_.ClearBlockers();
    validator_pool_.join_all();
    consensus_pool_.join_all();
  }

  /* Disallow copy and assign */
  DevcashControllerWorker(DevcashControllerWorker const&) = delete;
  DevcashControllerWorker& operator=(DevcashControllerWorker const&) = delete;

  void Start();

  void StartToy();

  /** Stops all threads in this pool.
   * @note This function may block.
   * @return true iff all threads in this pool joined.
   * @return false if some error occurred.
   */
  bool StopAll();

  void pushValidator(std::unique_ptr<DevcashMessage> message);

  void pushConsensus(std::unique_ptr<DevcashMessage> message);

  void ValidatorLoop();

  void ConsensusLoop();

 private:
  const int validator_num_ = kDEFAULT_WORKERS;
  const int consensus_num_ = kDEFAULT_WORKERS;
  boost::thread_group validator_pool_;
  boost::thread_group consensus_pool_;
  DevcashMPMCQueue validators_;
  DevcashMPMCQueue consensus_;
  std::atomic<bool> continue_;  //signals all threads to stop gracefully
  DevcashController* controller_;
  bool toy_mode_ = false;

};

} /* namespace Devcash */

#endif /* CONCURRENCY_DEVCASHWORKER_H_ */
