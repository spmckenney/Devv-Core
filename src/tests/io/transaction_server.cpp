#include <unistd.h>
#include <thread>
#include <chrono>

#include "types/DevcashMessage.h"
#include "io/message_service.h"
#include "io/constants.h"

int
main(int argc, char** argv) {

  auto dev_message = std::make_unique<Devcash::DevcashMessage>();
  dev_message->index = 10;
  dev_message->uri = "Hello!";
  dev_message->message_type = Devcash::eMessageType::VALID;
  dev_message->data.emplace_back(25);

  Devcash::LogDevcashMessageSummary(*dev_message);

  auto buffer = Devcash::serialize(*dev_message);

  auto new_message = Devcash::deserialize(buffer);

  assert(dev_message->index == new_message->index);

  Devcash::LogDevcashMessageSummary(*new_message);
  LOG_INFO << "We made it!!!";

  if (argc < 2) {
    LOG_ERROR << "Usage: " << argv[0] << " bind-endpoint [URI]";
    LOG_ERROR << "ex: " << argv[0] << " tcp://*:55557 my-message";
    exit(-1);
  }

  std::string bind_endpoint = argv[1];

  LOG_INFO << "Binding to " << bind_endpoint;

  std::string message;
  if (argc > 2) {
    message = argv[2];
  } else {
    message = "my-uri";
  }


  // Zmq Context
  zmq::context_t context(1);

  Devcash::io::TransactionServer server{context, bind_endpoint};

  server.StartServer();

  for (;;) {
    sleep(1);
    auto devcash_message = Devcash::DevcashMessageUniquePtr(new Devcash::DevcashMessage);
    devcash_message->uri = message;
    devcash_message->message_type = Devcash::eMessageType::VALID;
    LOG_INFO << "Sending message: " << message;
    server.QueueMessage(std::move(devcash_message));
  }

  return 0;
}
