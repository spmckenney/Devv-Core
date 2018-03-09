#include <fbzmq/zmq/Zmq.h>
#include <glog/logging.h>

#include <fbzmq/async/StopEventLoopSignalHandler.h>
#include "io/message_service.h"
#include "io/constants.h"

void print_devcash_message(Devcash::DevcashMessageSharedPtr message) {
  LOG(INFO) << "Got a message";
  LOG(INFO) << "DC->uri: " << message->uri;
  return;
}
  

int
main(int argc, char** argv) {
  //gflags::ParseCommandLineFlags(&argc, &argv, true);
  //google::InitGoogleLogging(argv[0]);
  //google::InstallFailureSignalHandler();

  // start signal handler before any thread
  fbzmq::ZmqEventLoop mainEventLoop;
  fbzmq::StopEventLoopSignalHandler handler(&mainEventLoop);
  //handler.registerSignalHandler(SIGINT);
  //handler.registerSignalHandler(SIGQUIT);
  //handler.registerSignalHandler(SIGTERM);

  std::vector<std::thread> allThreads{};

  // Zmq Context
  fbzmq::Context ctx;

  // start ZmqClient
  Devcash::io::TransactionClient client{ctx,
      Devcash::io::constants::kTransactionMessageUrl.str()};

  client.AttachCallback(print_devcash_message);

  std::thread clientThread([&client]() noexcept {
    LOG(INFO) << "Starting Client thread ...";
    client.run();
    LOG(INFO) << "Client stopped.";
  });

  client.waitUntilRunning();
  allThreads.emplace_back(std::move(clientThread));

  LOG(INFO) << "Starting main event loop...";
  mainEventLoop.run();
  LOG(INFO) << "Main event loop got stopped";

  client.stop();
  client.waitUntilStopped();

  for (auto& t : allThreads) {
    t.join();
  }
  return 0;
}
