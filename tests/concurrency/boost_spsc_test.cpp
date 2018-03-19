#include <boost/thread/thread.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <iostream>

#include "common/logger.h"
#include "common/util.h"
#include "types/DevcashMessage.h"

#include <boost/atomic.hpp>

const int kMessageCount = 100000;
using namespace Devcash;

int producer_count = 0;
boost::atomic_int consumer_count (0);

boost::lockfree::spsc_queue<DevcashMessage, boost::lockfree::capacity<1024> > spsc_queue;

const int iterations = 10000000;

DevcashMessage get_message() {
  std::vector<uint8_t> data(100);
  eMessageType msg{eMessageType::VALID};
  URI uri = "Hello";

  DevcashMessage d(uri, msg, data);
  return d;
}

DevcashMessage get_message_ptr() {
  std::vector<uint8_t> data(1000000);
  eMessageType msg{eMessageType::VALID};
  URI uri = "Hello";

  DevcashMessage d(uri, msg, data);
  return d;
}

struct BufferTester {
  bool operator()(DevcashMessage& message) {
    ++tot_count;
    if (!(tot_count % 1000)) {
      LOG_DEBUG << "Got " << int(tot_count/1000) << " messages";
    }
    if (message.message_type == eMessageType::VALID) {
      ++valid_count;
    }
    if (tot_count >= kMessageCount) {
      return true;
    } else {
      return false;
    }
  }
  int tot_count = 0;
  int valid_count = 0;
};

void producer(void)
{
      auto value = get_message();
    for (int i = 0; i != iterations; ++i) {
      //auto value = get_message();
        while (!spsc_queue.push(value))
            ;
    }
}

boost::atomic<bool> done (false);

void consumer(void)
{
  BufferTester t;
  DevcashMessage value;
    while (!done) {
        while (spsc_queue.pop(value))
            ++consumer_count;
        t(value);
    }

    while (spsc_queue.pop(value))
        ++consumer_count;
}

int main(int argc, char* argv[])
{
    using namespace std;
    cout << "boost::lockfree::queue is ";
    if (!spsc_queue.is_lock_free())
        cout << "not ";
    cout << "lockfree" << endl;

    boost::thread producer_thread(producer);
    boost::thread consumer_thread(consumer);

    producer_thread.join();
    done = true;
    consumer_thread.join();

    cout << "produced " << producer_count << " objects." << endl;
    cout << "consumed " << consumer_count << " objects." << endl;
}
