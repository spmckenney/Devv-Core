/*
 * DevCashRingQueue.h implements a one-way ring queue for pointers to
 * DevcashMessage objects.
 *
 * This RingQueue is designed for distinct Producers and Consumers.
 * Producers and Consumers should share a reference to the queue,
 * but no object should ever call both push and pop for a given queue.
 * This RingQueue is only intended to work in one direction.
 * For bi-directional communication, construct a second queue.
 *
 * Note that this object cannot prevent the Producer from retaining access
 * to the message after it is sent. Producers should never modify or access
 * the message after pushing it.
 *
 * The point lifecycle is as follows:
 * 1.  Producer creates DevcashMessage and unique_ptr to it.
 * 2.  Producer moves unique_ptr into queue using push().
 * 3.  Producer's pointer is now null and it should release the message.
 * 4.  Consumer uses pop() to get pointer from queue.
 * 5.  Queue pointer is now null
 * 6.  Consumer deletes DevcashMessage when it is finished with it.
 *
 *  Created on: Mar 14, 2018
 *      Author: Nick Williams
 */

#ifndef QUEUES_DEVCASHSPSCQUEUE_H_
#define QUEUES_DEVCASHSPSCQUEUE_H_

#include <array>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <thread>

#include <boost/lockfree/spsc_queue.hpp>

#include "types/DevcashMessage.h"
#include "common/logger.h"
#include "common/util.h"

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
      std::this_thread::sleep_for (std::chrono::milliseconds(1));
    }
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
    
    //LOG_DEBUG << "popped: " << ptr << " " << ptr->uri;
    std::unique_ptr<DevcashMessage> pointer(ptr);
    return pointer;
  }

 private:
  boost::lockfree::spsc_queue<DevcashMessage*, boost::lockfree::capacity<1024> > spsc_queue_;
};

} /* namespace Devcash */

#endif /* QUEUES_DEVCASHSPSCQUEUE_H_ */
