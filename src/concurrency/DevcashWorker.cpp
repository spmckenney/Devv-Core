/*
 * DevcashWorker.cpp
 *
 *  Created on: Mar 26, 2018
 *      Author: Nick Williams
 */

#include "DevcashWorker.h"

#include <functional>
#include <thread>

#include <boost/thread/thread.hpp>
#include <boost/asio/io_service.hpp>

#include "types/DevcashMessage.h"
#include "common/logger.h"
#include "common/util.h"
#include "DevcashController.h"
#include "DevcashRingQueue.h"

using namespace Devcash;

namespace Devcash {

  /* Constructors/Destructors */
  DevcashControllerWorker::DevcashControllerWorker(DevcashController* control,
                    const int validators=kDEFAULT_WORKERS,
                    const int consensus=kDEFAULT_WORKERS,
                    const int shard_comms=kDEFAULT_WORKERS)
     : validator_num_(validators), consensus_num_(consensus)
     , shardcomm_num_(shard_comms), continue_(true), controller_(control)
  {
  }

  void DevcashControllerWorker::Start() {
    CASH_TRY {
      for (int w = 0; w < validator_num_; w++) {
        validator_pool_.create_thread(
            boost::bind(&DevcashControllerWorker::ValidatorLoop, this));
      }
      for (int w = 0; w < consensus_num_; w++) {
        consensus_pool_.create_thread(
            boost::bind(&DevcashControllerWorker::ConsensusLoop, this));
      }
      for (int w = 0; w < consensus_num_; w++) {
        shardcomm_pool_.create_thread(
            boost::bind(&DevcashControllerWorker::ShardCommsLoop, this));
      }
    } CASH_CATCH (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "Worker.start");
    }
  }

  void DevcashControllerWorker::StartToy() {
    CASH_TRY {
      toy_mode_ = true;
      Start();
    } CASH_CATCH (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "Worker.startToy");
    }
  }

  /** Stops all threads in this pool.
   * @note This function may block.
   * @return true iff all threads in this pool joined.
   * @return false if some error occurred.
   */
  bool DevcashControllerWorker::StopAll() {
    LOG_DEBUG << "DevcashControllerWorker::stopAll()";
    if (!continue_) return false;
    CASH_TRY {
      continue_ = false;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      validators_.ClearBlockers();
      consensus_.ClearBlockers();
      shardcomm_.ClearBlockers();
      validator_pool_.join_all();
      consensus_pool_.join_all();
      shardcomm_pool_.join_all();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      return true;
    } CASH_CATCH (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "Worker.stopAll");
      return false;
    }
  }

  void DevcashControllerWorker::pushValidator(std::unique_ptr<DevcashMessage> message) {
    LOG_DEBUG << "DevcashControllerWorker::pushValidator()";
    CASH_TRY {
      validators_.push(std::move(message));
    } CASH_CATCH (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "Worker.push");
    }
  }

  void DevcashControllerWorker::pushConsensus(std::unique_ptr<DevcashMessage> message) {
    LOG_DEBUG << "DevcashControllerWorker::pushConsensus()";
    CASH_TRY {
      consensus_.push(std::move(message));
    } CASH_CATCH (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "Worker.push");
    }
  }

  void DevcashControllerWorker::pushShardComms(std::unique_ptr<DevcashMessage> message) {
    LOG_DEBUG << "DevcashControllerWorker::pushShardComms()";
    CASH_TRY {
      shardcomm_.push(std::move(message));
    } CASH_CATCH (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "Worker.push");
    }
  }

  void DevcashControllerWorker::ValidatorLoop() {
    LOG_DEBUG << "DevcashControllerWorker::ValidatorLoop(): Validator Ready";
    CASH_TRY {
      while (continue_) {
        if (!toy_mode_) {
            controller_->validatorCallback(std::move(validators_.pop()));
        } else {
          controller_->validatorToyCallback(std::move(validators_.pop()));
        }
      }
    } CASH_CATCH (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "ControllerWorker.ValidatorLoop");
    }
  }

  void DevcashControllerWorker::ConsensusLoop() {
    LOG_DEBUG << "DevcashControllerWorker::ConsensusLoop(): Consensus Worker Ready";
    CASH_TRY {
      while (continue_) {
        if (!toy_mode_) {
          controller_->consensusCallback(std::move(consensus_.pop()));
        } else {
          controller_->consensusToyCallback(std::move(consensus_.pop()));
        }
      }
    } CASH_CATCH (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "ControllerWorker.ConsensusLoop");
    }
  }

  void DevcashControllerWorker::ShardCommsLoop() {
    LOG_DEBUG << "DevcashControllerWorker::ShardCommsLoop(): Shard Comms Worker Ready";
    CASH_TRY {
      while (continue_) {
        if (!toy_mode_) {
          controller_->shardCommsCallback(std::move(consensus_.pop()));
        }
      }
    } CASH_CATCH (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "ControllerWorker.ShardCommsLoop");
    }
  }

} /* namespace Devcash */
