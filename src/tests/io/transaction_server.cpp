#include <unistd.h>
#include <thread>
#include <chrono>

#include "types/DevcashMessage.h"
#include "io/message_service.h"
#include "io/constants.h"

int
main(int argc, char** argv) {

  // start signal handler before any thread
  /*
  fbzmq::ZmqEventLoop mainEventLoop;
  fbzmq::StopEventLoopSignalHandler handler(&mainEventLoop);
  handler.registerSignalHandler(SIGINT);
  handler.registerSignalHandler(SIGQUIT);
  handler.registerSignalHandler(SIGTERM);

  std::vector<std::thread> allThreads{};
  */

  // Zmq Context
  zmq::context_t context(1);

  Devcash::io::TransactionServer server{context, "tcp://*:55557"};

  /*
  std::thread serverThread([&server]() noexcept {
    LOG(info) << "Starting Server thread ...";
    server.run();
    LOG(info) << "Server stopped.";
  });

  server.waitUntilRunning();
  allThreads.emplace_back(std::move(serverThread));

  LOG(info) << "Creating Event Thread";;
  std::thread event_loop_thread([&mainEventLoop, &server]() noexcept {
      LOG(info) << "Starting main event loop...";
      mainEventLoop.run();
      LOG(info) << "Main event loop got stopped";

      server.stop();
      server.waitUntilStopped();
    });
  LOG(info) << "Creating Event Thread";;

  allThreads.emplace_back(std::move(event_loop_thread));
  */

  server.StartServer();

  LOG(info) << "Gonna sleep(1)";

  for (;;) {
    sleep(1);
    auto devcash_message = Devcash::DevcashMessageUniquePtr(new Devcash::DevcashMessage);
    devcash_message->uri = "my_uri2";
    devcash_message->message_type = Devcash::eMessageType::VALID;
    LOG(info) << "Sending message";
    server.QueueMessage(std::move(devcash_message));
  }

  /*
  for (auto& t : allThreads) {
    t.join();
  }
  */
  return 0;
}
