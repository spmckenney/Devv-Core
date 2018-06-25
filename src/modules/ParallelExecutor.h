/*
 * ParallelExecutor.h
 *
 *  Created on: 6/21/18
 *      Author: Shawn McKenney <shawn.mckenney@emmion.com>
 */
#pragma once

#include "concurrency/ThreadGroup.h"
#include "types/DevcashMessage.h"

namespace Devcash {

/**
 * The ParallelExecutor is a template class that can execute
 * many instances of the Controller algorithm in parallel.
 * @tparam Controller
 */
template <typename Controller>
class ParallelExecutor {
 public:
  /**
   * Constructor. Takes a reference to a controller object of type Controller, the context
   * and the number of parallel instances to run.
   *
   * @param controller
   * @param context
   * @param num_threads
   */
  ParallelExecutor(Controller& controller,
                     DevcashContext& context,
                     size_t num_threads)
      : controller_(controller)
      , context_(context)
      , thread_group_(num_threads)
  {
    LOG_DEBUG << "ParallelExecutor(" << num_threads << ")";
  }

  /**
   * Attach a callback to be executed in parallel
   * @param callback
   */
  void attachCallback(DevcashMessageCallback callback) {
    if (callback == nullptr) {
      throw std::runtime_error("Cannot attach a nullptr callback");
    }
    LOG_DEBUG << "attachCallback()";
    thread_group_.attachCallback(callback);
  }

  /**
   * Initializes the executor and calls init() on the controller algorithm.
   */
  void init() {
    LOG_DEBUG << "init()";
    controller_.init();
  }

  /**
   * Performs a set of sanity checks to ensure the controller is ready to run.
   * Calls sanityChecks() on the controller. An exception is thrown if the
   * sanity checks fail.
   */
  void sanityChecks() {
    LOG_DEBUG << "sanityChecks()";
    controller_.sanityChecks();
  }

  /**
   * Fires up the threads by calling start() on the thread group
   */
  void start() {
    LOG_DEBUG << "start()";
    thread_group_.start();
  }

  void shutdown () {
    LOG_DEBUG << "shutdown()";
    thread_group_.stop();
  }

  /**
   * Pushes a message to the ThreadGroup queue
   * @param message
   */
  void pushMessage(DevcashMessageUniquePtr message) {
    LOG_DEBUG << "pushMessage(), do not segfault";
    //thread_group_.pushMessage(std::move(message));
  }

 private:
  Controller& controller_;
  DevcashContext& context_;
  ThreadGroup thread_group_;
};

} // namespace Devcash
