/*
 * ThreadGroup.cpp
 *
 *  Created on: June 21, 2018
 *      Author: Shawn McKenney
 */
#include "ThreadGroup.h"
#include "common/logger.h"

namespace Devcash {

void ThreadGroup::attachCallback(DevcashMessageCallback callback) {
  message_callback_ = callback;
}

void ThreadGroup::pushMessage(DevcashMessageUniquePtr message) {
  thread_queue_.push(std::move(message));
}

bool ThreadGroup::start() {
  if (!do_run_) {
    do_run_ = true;
    LOG_INFO << "ThreadGroup::start()";
    for (int w = 0; w < num_threads_; w++) {
      thread_pool_.create_thread(
          boost::bind(&ThreadGroup::loop, this));
    }
    return true;
  } else {
    LOG_ERROR << "ThreadGroup::start() - threads already running";
    return false;
  }
}

bool ThreadGroup::stop() {
  LOG_DEBUG << "ThreadGroup::stop()";
  if (!do_run_) {
    return false;
  } else {
    do_run_ = false;
    std::this_thread::sleep_for(std::chrono::milliseconds(kMAIN_WAIT_INTERVAL));
    thread_queue_.ClearBlockers();
    thread_pool_.join_all();
    std::this_thread::sleep_for(std::chrono::milliseconds(kMAIN_WAIT_INTERVAL));
    return true;
  }
}

void ThreadGroup::loop() {
  LOG_INFO << "ThreadGroup::loop()";
  while (do_run_) {
    message_callback_(std::move(thread_queue_.pop()));
  }
  LOG_INFO << "ThreadGroup::loop() exit";
}

} // namespace Devcash