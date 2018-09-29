/*
 * DevcashWorker.h
 *
 *  Created on: Mar 26, 2018
 *      Author: Nick Williams
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
#include "ValidatorController.h"
#include "DevcashMPMCQueue.h"

namespace Devcash {

class DevcashControllerWorker {
 public:

  /* Constructors/Destructors */
  DevcashControllerWorker(ValidatorController* control,
                    const int validators, const int consensus, const int shard_comms);

  virtual ~DevcashControllerWorker() {
    StopAll();
  }

  /* Disallow copy and assign */
  DevcashControllerWorker(DevcashControllerWorker const&) = delete;
  DevcashControllerWorker& operator=(DevcashControllerWorker const&) = delete;

  void Start();

  /** Stops all threads in this pool.
   * @note This function may block.
   * @return true iff all threads in this pool joined.
   * @return false if some error occurred.
   */
  bool StopAll();

  void pushValidator(std::unique_ptr<DevcashMessage> message);

  void pushConsensus(std::unique_ptr<DevcashMessage> message);

  void pushShardComms(std::unique_ptr<DevcashMessage> message);

  void ValidatorLoop();

  void ConsensusLoop();

  void ShardCommsLoop();

 private:
  const int validator_num_ = kDEFAULT_WORKERS;
  const int consensus_num_ = kDEFAULT_WORKERS;
  const int shardcomm_num_ = kDEFAULT_WORKERS;
  boost::thread_group validator_pool_;
  boost::thread_group consensus_pool_;
  boost::thread_group shardcomm_pool_;
  DevcashMPMCQueue validators_;
  DevcashMPMCQueue consensus_;
  DevcashMPMCQueue shardcomm_;
  std::atomic<bool> continue_;  //signals all threads to stop gracefully
  ValidatorController* controller_;
  bool toy_mode_ = false;

};

}  // namespace Devcash

#endif /* CONCURRENCY_DEVCASHWORKER_H_ */
