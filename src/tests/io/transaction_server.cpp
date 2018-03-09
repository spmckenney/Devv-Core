#include <unistd.h>
#include <thread>
#include <chrono>

#include <fbzmq/zmq/Zmq.h>

#include <fbzmq/async/StopEventLoopSignalHandler.h>
#include "io/message_service.h"
#include "io/constants.h"

int
main(int argc, char** argv) {

  // start signal handler before any thread
  fbzmq::ZmqEventLoop mainEventLoop;
  fbzmq::StopEventLoopSignalHandler handler(&mainEventLoop);
  handler.registerSignalHandler(SIGINT);
  handler.registerSignalHandler(SIGQUIT);
  handler.registerSignalHandler(SIGTERM);

  std::vector<std::thread> allThreads{};

  // Zmq Context
  fbzmq::Context ctx;

  Devcash::io::TransactionServer server{ctx,
      Devcash::io::constants::kTransactionMessageUrl.str()};

  std::thread serverThread([&server]() noexcept {
    LOG(INFO) << "Starting Server thread ...";
    server.run();
    LOG(INFO) << "Server stopped.";
  });

  server.waitUntilRunning();
  allThreads.emplace_back(std::move(serverThread));

  LOG(INFO) << "Creating Event Thread";;
  std::thread event_loop_thread([&mainEventLoop, &server]() noexcept {
      LOG(INFO) << "Starting main event loop...";
      mainEventLoop.run();
      LOG(INFO) << "Main event loop got stopped";

      server.stop();
      server.waitUntilStopped();
    });
  LOG(INFO) << "Creating Event Thread";;

  allThreads.emplace_back(std::move(event_loop_thread));

  LOG(INFO) << "Gonna sleep(10)";
  sleep(10);
  auto devcash_message = std::make_shared<Devcash::DevcashMessage>();
  devcash_message->uri = "my_uri";
  devcash_message->message_type = Devcash::MessageType::eProposalBlock;
  devcash_message->data = std::make_shared<Devcash::DataBuffer>();
  LOG(INFO) << "Sending message";
  server.SendMessage(devcash_message);

  for (auto& t : allThreads) {
    t.join();
  }
  return 0;
}
