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
#include "ValidatorController.h"
#include "DevcashRingQueue.h"

using namespace Devcash;

namespace Devcash {

  /* Constructors/Destructors */
  DevcashControllerWorker::DevcashControllerWorker(ValidatorController* control,
                    const int validators=kDEFAULT_WORKERS,
                    const int consensus=kDEFAULT_WORKERS,
                    const int shard_comms=kDEFAULT_WORKERS)
     : validator_num_(validators), consensus_num_(consensus)
     , shardcomm_num_(shard_comms), continue_(true), controller_(control)
  {
  }

  void DevcashControllerWorker::Start() {
    try {
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
    } catch (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "Worker.start");
    }
  }

  bool DevcashControllerWorker::StopAll() {
    LOG_DEBUG << "DevcashControllerWorker::stopAll()";
    if (!continue_) return false;
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
  }

  void DevcashControllerWorker::pushValidator(std::unique_ptr<DevcashMessage> message) {
    LOG_DEBUG << "DevcashControllerWorker::pushValidator()";
    try {
      validators_.push(std::move(message));
    } catch (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "Worker.push");
    }
  }

  void DevcashControllerWorker::pushConsensus(std::unique_ptr<DevcashMessage> message) {
    LOG_DEBUG << "DevcashControllerWorker::pushConsensus()";
    try {
      consensus_.push(std::move(message));
    } catch (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "Worker.push");
    }
  }

  void DevcashControllerWorker::pushShardComms(std::unique_ptr<DevcashMessage> message) {
    LOG_DEBUG << "DevcashControllerWorker::pushShardComms()";
    try {
      shardcomm_.push(std::move(message));
    } catch (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "DevcashControllerWorker::pushShardComms()");
    }
  }

  void DevcashControllerWorker::ValidatorLoop() {
    LOG_DEBUG << "DevcashControllerWorker::ValidatorLoop(): Validator Ready";
    try {
      while (continue_) {
        controller_->ValidatorCallback(std::move(validators_.pop()));
      }
    } catch (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "ControllerWorker.ValidatorLoop");
      throw;
    }
  }

  void DevcashControllerWorker::ConsensusLoop() {
    LOG_DEBUG << "DevcashControllerWorker::ConsensusLoop(): Consensus Worker Ready";
    try {
      while (continue_) {
        controller_->ConsensusCallback(std::move(consensus_.pop()));
      }
    } catch (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "ControllerWorker.ConsensusLoop");
      throw;
    }
  }

  void DevcashControllerWorker::ShardCommsLoop() {
    LOG_DEBUG << "DevcashControllerWorker::ShardCommsLoop(): Shard Comms Worker Ready";
    try {
      while (continue_) {
        if (!toy_mode_) {
          controller_->ShardCommsCallback(std::move(shardcomm_.pop()));
        }
      }
    } catch (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "ControllerWorker.ShardCommsLoop");
    }
  }

} /* namespace Devcash */
