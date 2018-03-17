#include <memory>
#include "queues/DevcashRingQueue.h"
#include "types/DevcashMessage.h"

const int kMessageCount = 100000;
using namespace Devcash;

std::unique_ptr<DevcashMessage> get_unique() {
  std::vector<uint8_t> data(100);
  eMessageType msg{eMessageType::VALID};
  URI uri = "Hello";
  
  auto ptr = std::unique_ptr<DevcashMessage>(new DevcashMessage(uri, msg, data));
  return ptr;
}

void
put_unique(std::unique_ptr<DevcashMessage> message) {
}

struct BufferTester {
  bool operator()(std::unique_ptr<DevcashMessage> message) {
    ++tot_count;
    if (!(tot_count % 1000)) {
      LOG_DEBUG << "Got " << int(tot_count/1000) << " messages";
    }
    if (message->message_type == eMessageType::VALID) {
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

int
main(int, char**) {

  LOG_DEBUG << "Going to run!!";
  
  Devcash::DevcashRingQueue rq{};
  BufferTester func;
  
  auto msg = get_unique();
  
  std::thread reader([&rq, &func] {
      bool quit = false;
      LOG_DEBUG << "Going to pop!!";
      do {
        quit = func(rq.pop());
      } while (!quit);
    });

  for (auto i = 0; i < kMessageCount; i++) {
    if (!(i % 1000)) {
      LOG_DEBUG << "Sent " << int(i/1000) << " messages";
    }
    auto msg = get_unique();
    rq.push(std::move(msg));
  }

  LOG_DEBUG << "Waiting...";
  reader.join();

  LOG_DEBUG << "Done";

  return 0;
};
