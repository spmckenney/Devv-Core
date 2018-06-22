/*
 * ThreadedController.h
 *
 *  Created on: 6/21/18
 *      Author: Shawn McKenney <shawn.mckenney@emmion.com>
 */
#pragma once

#include "concurrency/ThreadGroup.h"
#include "types/DevcashMessage.h"

namespace Devcash {

template <typename Module>
class ThreadedController {
 public:
  ThreadedController(Module& module, DevcashContext& context)
      : module_(module), context_(context)
  {
  }


  void attachCallback(DevcashMessageCallback callback) {
    thread_group_.attachCallback(callback);
  }

  void init() {
    module_.init();
  }

  void sanityChecks() {
    module_.sanityChecks();
  }

  void start() {
    thread_group_.start();
  }

  void startShutdown() {};

  void shutdown () {
    thread_group_.stop();
  }

  /**
   * Pushes a message to the ThreadGroup queue
   * @param message
   */
  void pushMessage(DevcashMessageUniquePtr message) {
    thread_group_.pushMessage(std::move(message));
  }

 private:
  ThreadGroup thread_group_;
  Module& module_;
  DevcashContext& context_;
};

} // namespace Devcash
