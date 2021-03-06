/*
 * DevCashSPSCQueue.h implements a ring queue for pointers to Devcash messages.
 *
 *  Created on: Mar 17, 2018
 *      Author: Shawn McKenny
 */

#ifndef CONCURRENCY_DEVCASHSPSCQUEUE_H_
#define CONCURRENCY_DEVCASHSPSCQUEUE_H_

#include <array>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <thread>

#include <boost/lockfree/spsc_queue.hpp>

#include "types/DevcashMessage.h"
#include "common/logger.h"

namespace Devcash {

/**
 * Single producer / single consumer queue.
 */
class DevcashSPSCQueue {
 public:
  /* Constructors/Destructors */
  DevcashSPSCQueue() {
  }
  virtual ~DevcashSPSCQueue() {};

  /* Disallow copy and assign */
  DevcashSPSCQueue(DevcashSPSCQueue const&) = delete;
  DevcashSPSCQueue& operator=(DevcashSPSCQueue const&) = delete;

  /** Push the pointer for a message to this queue.
   * @note once the pointer is moved the old variable becomes a nullptr.
   * @params pointer the unique_ptr for the message to send
   * @return true iff the unique_ptr was queued
   * @return false otherwise
   */
  bool push(std::unique_ptr<DevcashMessage> pointer) {
    DevcashMessage* ptr = pointer.release();
    while (!spsc_queue_.push(ptr)) {
      std::this_thread::sleep_for (std::chrono::milliseconds(1));
    }
    return true;
  }

  /** Pop a message pointer from this queue.
   * @return unique_ptr to a DevcashMessage once one is queued.
   * @return nullptr, if any error
   */
  std::unique_ptr<DevcashMessage> pop() {
    DevcashMessage* ptr;

    while (!spsc_queue_.pop(ptr)) {
      std::this_thread::sleep_for (std::chrono::milliseconds(1));
    }

    std::unique_ptr<DevcashMessage> pointer(ptr);
    return pointer;
  }

 private:
  boost::lockfree::spsc_queue<DevcashMessage*, boost::lockfree::capacity<1024> > spsc_queue_;
};

} /* namespace Devcash */

#endif /* CONCURRENCY_DEVCASHSPSCQUEUE_H_ */
