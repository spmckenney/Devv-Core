/*
 * DevCashMPMCQueue.h implements a ring queue for pointers to Devcash messages.
 *
 *  Created on: Mar 17, 2018
 *      Author: Shawn McKenny
 */

#ifndef CONCURRENCY_DEVCASHMPMCQUEUE_H_
#define CONCURRENCY_DEVCASHMPMCQUEUE_H_

#include <array>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <thread>

#include "blockingconcurrentqueue.h"

#include "types/DevcashMessage.h"
#include "common/devcash_constants.h"
#include "common/logger.h"

namespace Devcash {

/**
 * Multiple producer / multiple consumer queue.
 */
class DevcashMPMCQueue {
 public:
  /* Constructors/Destructors */
  DevcashMPMCQueue(size_t capacity = kDEFAULT_WORKERS)
    : queue_(capacity) {
  }
  virtual ~DevcashMPMCQueue() {};

  /* Disallow copy and assign */
  DevcashMPMCQueue(DevcashMPMCQueue const&) = delete;
  DevcashMPMCQueue& operator=(DevcashMPMCQueue const&) = delete;

  /** Push the pointer for a message to this queue.
   * @note once the pointer is moved the old variable becomes a nullptr.
   * @params pointer the unique_ptr for the message to send
   * @return true iff the unique_ptr was queued
   * @return false otherwise
   */
  bool push(DevcashMessageUniquePtr pointer) {
    DevcashMessage* ptr = pointer.release();
    //LOG_DEBUG << "pushing: " << ptr << " " << ptr->uri;
    while (!queue_.enqueue(ptr)) {
      //std::this_thread::sleep_for (std::chrono::milliseconds(1));
    }
    return true;
  }

  /** Pop a message pointer from this queue.
   * @return unique_ptr to a DevcashMessage once one is queued.
   * @return nullptr, if any error
   */
  DevcashMessageUniquePtr pop() {
    DevcashMessage* ptr = nullptr;

    // Keep looping while wait is false (timeout) and keep_popping is true
    while (!queue_.wait_dequeue_timed(ptr, std::chrono::milliseconds(5))) {
      if (!keep_popping_) {
        LOG_INFO << "keep_popping_ == false";
        return nullptr;
      }
    }

    LOG_DEBUG << "DevcashMPMCQueue::pop()ped: " << ptr << " " << ptr->uri;
    std::unique_ptr<DevcashMessage> pointer(ptr);
    return pointer;
  }

  void ClearBlockers() {
    keep_popping_ = false;
  }

 private:
  moodycamel::BlockingConcurrentQueue<DevcashMessage*> queue_;
  bool keep_popping_ = true;
};

} /* namespace Devcash */

#endif /* CONCURRENCY_DEVCASHMPMCQUEUE_H_ */
