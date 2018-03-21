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

class DevcashSPSCQueue {
 public:
  //static const int kDEFAULT_RING_SIZE = 1024;

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
    //LOG_DEBUG << "pushing: " << ptr << " " << ptr->uri;
    while (!spsc_queue_.push(ptr)) {
      //std::this_thread::sleep_for (std::chrono::milliseconds(1));
    }
  }

  /** Pop a message pointer from this queue.
   * @return unique_ptr to a DevcashMessage once one is queued.
   * @return nullptr, if any error
   */
  std::unique_ptr<DevcashMessage> pop() {
    DevcashMessage* ptr;

    while (!spsc_queue_.pop(ptr)) {
      //std::this_thread::sleep_for (std::chrono::milliseconds(1));
    }

    //LOG_DEBUG << "popped: " << ptr << " " << ptr->uri;
    std::unique_ptr<DevcashMessage> pointer(ptr);
    return pointer;
  }

 private:
  boost::lockfree::spsc_queue<DevcashMessage*, boost::lockfree::capacity<1024> > spsc_queue_;
};

} /* namespace Devcash */

#endif /* CONCURRENCY_DEVCASHSPSCQUEUE_H_ */
