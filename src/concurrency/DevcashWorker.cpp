/*
 * DevcashWorker.cpp
 *
 *  Created on: Mar 26, 2018
 *      Author: Silver
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

namespace Devcash {

using namespace Devcash;

//exception toggling capability
#if (defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)) && not defined(DEVCASH_NOEXCEPTION)
    #define CASH_THROW(exception) throw exception
    #define CASH_TRY try
    #define CASH_CATCH(exception) catch(exception)
#else
    #define CASH_THROW(exception) std::abort()
    #define CASH_TRY if(true)
    #define CASH_CATCH(exception) if(false)
#endif

  /* Constructors/Destructors */
  DevcashControllerWorker::DevcashControllerWorker(DevcashController* control,
                    const int validators=kDEFAULT_WORKERS,
                    const int consensus=kDEFAULT_WORKERS)
     : validator_num_(validators), consensus_num_(consensus)
     , validators_(validators), consensus_(consensus)
     , continue_(true), controller_(control)
  {
  }

  void DevcashControllerWorker::start() {
    CASH_TRY {
      /*for (int w = 0; w < validator_num_; w++) {
        validator_pool_.create_thread(
            boost::bind(&DevcashControllerWorker::ValidatorLoop, this));
      }
      for (int w = 0; w < consensus_num_; w++) {
        consensus_pool_.create_thread(
            boost::bind(&DevcashControllerWorker::ConsensusLoop, this));
      }*/
    } CASH_CATCH (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "Worker.start");
    }
  }

  void DevcashControllerWorker::startToy() {
    CASH_TRY {
      toy_mode_ = true;
      start();
    } CASH_CATCH (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "Worker.startToy");
    }
  }

  /** Stops all threads in this pool.
   * @note This function may block.
   * @return true iff all threads in this pool joined.
   * @return false if some error occurred.
   */
  bool DevcashControllerWorker::stopAll() {
    CASH_TRY {
      continue_ = false;
      validators_.clearBlockers();
      consensus_.clearBlockers();
      validator_pool_.join_all();
      consensus_pool_.join_all();
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
      //controller_->ValidatorCallback(std::move(message));
    } CASH_CATCH (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "Worker.push");
    }
  }

  void DevcashControllerWorker::pushConsensus(std::unique_ptr<DevcashMessage> message) {
    LOG_DEBUG << "DevcashControllerWorker::pushConsensus()";
    CASH_TRY {
      (std::move(message));
      consensus_.push(std::move(message));
      //controller_->ConsensusCallback(std::move(message));
    } CASH_CATCH (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "Worker.push");
    }
  }

  void DevcashControllerWorker::ValidatorLoop() {
    LOG_DEBUG << "DevcashControllerWorker::ValidatorLoop(): Validator Ready";
    CASH_TRY {
      while (continue_) {
        validators_.popGuard();
        if (!continue_) break;
        if (!toy_mode_) {
          controller_->ValidatorCallback(std::move(validators_.pop()));
        } else {
          controller_->ValidatorToyCallback(std::move(validators_.pop()));
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
        consensus_.popGuard();
        if (!continue_) break;
        if (!toy_mode_) {
          controller_->ConsensusCallback(std::move(consensus_.pop()));
        } else {
          controller_->ConsensusToyCallback(std::move(consensus_.pop()));
        }
      }
    } CASH_CATCH (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "ControllerWorker.ConsensusLoop");
    }
  }

} /* namespace Devcash */
