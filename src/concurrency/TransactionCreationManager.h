#ifndef CONCURRENCY_TRANSACTIONCREATIONMANAGER_H_
#define CONCURRENCY_TRANSACTIONCREATIONMANAGER_H_

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread_pool.hpp>
#include <boost/thread.hpp>

#include "primitives/Tier1Transaction.h"
#include "primitives/Tier2Transaction.h"

using namespace Devcash;

namespace Devcash
{

typedef boost::packaged_task<bool> task_t;
typedef boost::shared_ptr<task_t> ptask_t;

static inline void push_job(Transaction& x
              , const KeyRing& keyring
              , boost::asio::io_service& io_service
              , std::vector<boost::shared_future<bool>>& pending_data) {
  ptask_t task = boost::make_shared<task_t>([&x, &keyring]() {
      x.setIsSound(keyring);
      return(true);
    });
  boost::shared_future<bool> fut(task->get_future());
  pending_data.push_back(fut);
  io_service.post(boost::bind(&task_t::operator(), task));
}

class TransactionCreationManager {
public:
  TransactionCreationManager(TransactionCreationManager&) = delete;

  TransactionCreationManager(eAppMode mode)
    : io_service_()
    , threads_()
    , work_(io_service_)
    , app_mode_(mode)
  {
    for (unsigned int i = 0; i < boost::thread::hardware_concurrency(); ++i)
    {
        threads_.create_thread(boost::bind(&boost::asio::io_service::run,
                                          &io_service_));
    }
  }

  /**
   * Create Transactions objects from serialized binary data.
   * @param serial - a binary series of Transactions
   * @param vtx - (out) target vector for pointers to the Transaction objects
   * @param offset - a offset where this function should start reading the serial data
   * @param min_size - the minimum number of bytes in a Transaction
   * @param tx_size - the total byte size of this block of serial Transactions
   */
  void CreateTransactions(const std::vector<byte>& serial
                          , std::vector<TransactionPtr>& vtx
                          , size_t& offset
                          , size_t min_size
                          , size_t& tx_size) {

    while (offset < (min_size + tx_size)) {
      //Transaction constructor increments offset by ref
      LOG_TRACE << "while, offset = " << offset;
      if (app_mode_ == eAppMode::T1) {
        Tier1TransactionPtr one_tx = std::make_unique<Tier1Transaction>(serial
                                                                        , offset
                                                                        , *keys_p_
                                                                        , false);
        vtx.push_back(std::move(one_tx));
      } else if (app_mode_ == eAppMode::T2) {
        Tier2TransactionPtr one_tx = std::make_unique<Tier2Transaction>(serial
                                                                        , offset
                                                                        , *keys_p_
                                                                        , false);
        vtx.push_back(std::move(one_tx));
      }
    }

    std::vector<boost::shared_future<bool>> pending_data; // vector of futures

    for (auto& tx : vtx) {
      push_job(*tx, *keys_p_, io_service_, pending_data);
    }

    boost::wait_for_all(pending_data.begin(), pending_data.end());
  }

  /**
   * Set a pointer to the directory of keys and address.
   * @param keys - a pointer to the directory of keys and addresses.
   */
  void set_keys(const KeyRing* keys) {
    keys_p_ = keys;
  }

 private:
  const KeyRing* keys_p_ = nullptr;
  boost::asio::io_service io_service_;
  boost::thread_group threads_;
  boost::asio::io_service::work work_;
  const eAppMode app_mode_;
};

}

#endif /* CONCURRENCY_TRANSACTIONCREATIONMANAGER_H_ */
