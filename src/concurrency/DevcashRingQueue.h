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

#ifndef QUEUES_DEVCASHRINGQUEUE_H_
#define QUEUES_DEVCASHRINGQUEUE_H_

#include <array>
#include <condition_variable>
#include <mutex>
#include <thread>

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

class DevcashRingQueue {
 public:
  static const int kDEFAULT_RING_SIZE = 10;

  /* Constructors/Destructors */
  DevcashRingQueue() :
    kRingSize_(kDEFAULT_RING_SIZE) {
  }
  virtual ~DevcashRingQueue() {};

  /* Disallow copy and assign */
  DevcashRingQueue(DevcashRingQueue const&) = delete;
  DevcashRingQueue& operator=(DevcashRingQueue const&) = delete;

  /** Push the pointer for a message to this queue.
   * @note once the pointer is moved the old variable becomes a nullptr.
   * @params pointer the unique_ptr for the message to send
   * @return true iff the unique_ptr was queued
   * @return false otherwise
   */
  bool push(std::unique_ptr<DevcashMessage> pointer) {
    CASH_TRY {
      std::unique_lock<std::mutex> lock(pushLock_);
      while (isFull_) {
        full_.wait(lock, [&]() {
          return(!isFull_);}
        );
      }
      std::unique_lock<std::mutex> eqLock(update_);
      ptrRing_[pushAt_] = std::move(pointer);
      isEmpty_ = false;
      empty_.notify_one();
      pushAt_++;
      if (pushAt_ >= kRingSize_) pushAt_ = 0;
      if (pushAt_ == popAt_) {
        isFull_ = true;
      } else {
        isFull_ = false;
        full_.notify_one();
      }
      return true;
    } CASH_CATCH (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "DevcashRingQueue.push");
      return false;
    }
  }

  /** Pop a message pointer from this queue.
   * @return unique_ptr to a DevcashMessage once one is queued.
   * @return nullptr, if any error
   */
  std::unique_ptr<DevcashMessage> pop() {
    CASH_TRY {
      std::unique_lock<std::mutex> lock(popLock_);
      while (isEmpty_) {
        empty_.wait(lock, [&]() {
          return(!isEmpty_);}
        );
      }
      std::unique_lock<std::mutex> eqLock(update_);
      std::unique_ptr<DevcashMessage> out = std::move(ptrRing_[popAt_]);
      isFull_ = false;
      full_.notify_one();
      popAt_++;
      if (popAt_ >= kRingSize_) popAt_ = 0;
      if (popAt_ == pushAt_) {
        isEmpty_ = true;
      } else {
        isEmpty_ = false;
        empty_.notify_one();
      }
      return out;
    } CASH_CATCH (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "DevcashRingQueue.pop");
      return nullptr;
    }
  }

 private:
  int kRingSize_;
  std::array<std::unique_ptr<DevcashMessage>, kDEFAULT_RING_SIZE> ptrRing_;
  int pushAt_ = 0;
  int popAt_ = 0;
  std::mutex pushLock_; //orders access to push()
  std::mutex popLock_; //orders access to pop()
  std::mutex update_; //stops threads from updating members simultaneously
  bool isEmpty_ = true;
  bool isFull_ = false;
  std::condition_variable empty_;
  std::condition_variable full_;
};

} /* namespace Devcash */

#endif /* QUEUES_DEVCASHRINGQUEUE_H_ */
