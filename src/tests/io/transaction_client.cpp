#include <vector>
#include <string>

#include <boost/program_options.hpp>

#include "io/message_service.h"
#include "io/constants.h"

#include "transaction_test_struct.h"

void print_devcash_message(Devcash::DevcashMessageUniquePtr message) {
  LOG(info) << "Got a message!";
  LogDevcashMessageSummary(*message, "transaction_client");

  test_struct test;
  message->GetData(test);
  LOG_INFO << "test_struct - a:" << test.a
           << " b:" << test.b
           << " c:" << test.c;
  return;
}


namespace po = boost::program_options;

int
main(int argc, char** argv) {

  if (argc < 2) {
    LOG(error) << "Usage: " << argv[0] << " endpoint";
    LOG(error) << "ex: " << argv[0] << " tcp://localhost:55557";
    exit(-1);
  }

  std::string endpoint = argv[1];

  std::string filter = "";
  if (argc > 2) {
    filter = argv[2];
  }

  LOG(info) << "Connecting to " << endpoint;

  // Zmq Context
  zmq::context_t context(1);

  // start ZmqClient
  Devcash::io::TransactionClient client{context};
  client.AddConnection(endpoint);
  client.AttachCallback(print_devcash_message);

  client.ListenTo(filter);

  client.Run();

  return 0;
}
