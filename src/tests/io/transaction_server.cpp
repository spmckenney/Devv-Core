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

  LOG(INFO) << "Starting main event loop...";
  mainEventLoop.run();
  LOG(INFO) << "Main event loop got stopped";

  server.stop();
  server.waitUntilStopped();

  for (auto& t : allThreads) {
    t.join();
  }
  return 0;
}
