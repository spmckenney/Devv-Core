#include <fbzmq/zmq/Zmq.h>

#include "io/message_service.h"
#include "io/constants.h"

int
main(int argc, char** argv) {

  // Zmq Context
  fbzmq::Context ctx;
  
  Devcash::io::TransactionServer ts{ctx, Devcash::io::constants::kTransactionMessageUrl.str()};

}
