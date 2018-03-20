#include <vector>
#include <string>
#include <iostream>
#include <exception>

#include <boost/program_options.hpp>

#include "common/logger.h"
#include "io/message_service.h"
#include "io/constants.h"

void print_devcash_message(Devcash::DevcashMessageUniquePtr message) {
  LOG(info) << "Got a message";
  LOG(info) << "DC->uri: " << message->uri;
  return;
}
  

namespace po = boost::program_options;

int
main(int argc, char** argv) {

  try {
    int opt;
    po::options_description desc("\n\
The t1_example program can be used to demonstrate connectivity of\n\
T1 and T2 networks. By default, it will send random DevcashMessage\n\
every 5 seconds to any other t1_example programs that have connected\n\
to it will receive the message. If it receives a message, it will\n\
execute the appropriate T1 or T2 callback. t1_example can connect\n\
and listen to multiple other t1_example programs so almost any size\n\
network could be build and tested.\n\nAllowed options");
    desc.add_options()
      ("help", "produce help message")
      ("t1-host-list", po::value<std::vector<std::string>>(), 
       "T1 client URI (i.e. tcp://192.168.10.1:5005). Option can be repeated to connect to multiple hosts.")
      ("t2-host-list", po::value<std::vector<std::string>>(), 
       "T2 client URI (i.e. tcp://192.168.10.1:5005). Option can be repeated to connect to multiple hosts.")
      ("t1-bind-endpoint", po::value<std::string>(), "Endpoint for T1 server (i.e. tcp://*:5556)")
      ("t2-bind-endpoint", po::value<std::string>(), "Endpoint for T2 server (i.e. tcp://*:5556)")
      ;

    po::variables_map vm;        
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);    

    if (vm.count("help")) {
      LOG(debug) << desc << "\n";
      return 0;
    }

    std::string t1_bind_endpoint{};
    if (vm.count("t1-bind-endpoint")) {
      LOG(debug) << "T1 bind URI:" 
                 << vm["t1-bind-endpoint"].as<std::string>();
    } else {
      LOG(debug) << "T1 bind URI was not set.\n";
    }

    std::string t2_bind_endpoint{};
    if (vm.count("t2-bind-endpoint")) {
      LOG(debug) << "T2 bind URI:" 
                 << vm["t2-bind-endpoint"].as<std::string>();
    } else {
      LOG(debug) << "T2 bind URI was not set.\n";
    }

    if (vm.count("t1-host")) {
      const std::vector<std::string>& t1_host_vec = vm["include-path"].as<std::vector<std::string>>();
      LOG(debug) << "T1 host URIs:";
      for (auto i : t1_host_vec) {
        LOG(debug) << "  " << i;
      }
    }
  }
  catch(std::exception& e) {
    LOG_ERROR << "error: " << e.what() << "\n";
    return 1;
  }
  catch(...) {
    LOG_ERROR << "Exception of unknown type!\n";
  }

  std::vector<std::thread> allThreads{};

  // Zmq Context
  zmq::context_t context(1);

  // start ZmqClient
  Devcash::io::TransactionClient client{context, "tcp://localhost:55556"};

  client.AttachCallback(print_devcash_message);

  client.Run();

  /*
  std::thread clientThread([&client]() noexcept {
    LOG(info) << "Starting Client thread ...";
    client.Run();
    LOG(info) << "Client stopped.";
  });
  */

  /*
  client.waitUntilRunning();
  allThreads.emplace_back(std::move(clientThread));

  LOG(info) << "Starting main event loop...";
  mainEventLoop.run();
  LOG(info) << "Main event loop got stopped";

  client.stop();
  client.waitUntilStopped();
  */
  
  /*
  for (auto& t : allThreads) {
    t.join();
  }
  */
  return 0;
}
