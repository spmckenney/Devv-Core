#include <unistd.h>
#include <thread>
#include <chrono>

#include "types/DevcashMessage.h"
#include "io/message_service.h"
#include "io/constants.h"

int
main(int argc, char** argv) {

  if (argc < 2) {
    LOG(error) << "Usage: " << argv[0] << " bind-endpoint [URI]";
    LOG(error) << "ex: " << argv[0] << " tcp://*:55557 my-message";
    exit(-1);
  }

  std::string bind_endpoint = argv[1];

  LOG(info) << "Binding to " << bind_endpoint;

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
    LOG(info) << "Sending message: " << message;
    server.QueueMessage(std::move(devcash_message));
  }

  return 0;
}
