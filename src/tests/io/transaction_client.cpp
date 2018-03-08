#include <fbzmq/zmq/Zmq.h>
#include <glog/logging.h>

#include "io/message_service.h"
#include "io/constants.h"

//void print_devcash_message(DevcashMessageSharedPtr message) {
  

int
main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();

  // Zmq Context
  fbzmq::Context ctx;

  // start ZmqClient
  Devcash::io::TransactionClient client{ctx,
      Devcash::io::constants::kTransactionMessageUrl.str()};

  return 0;
}
