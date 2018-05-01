#pragma once

#ifndef COMMON_ARGUMENT_PARSER_H_
#define COMMON_ARGUMENT_PARSER_H_

#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <exception>

#include <boost/program_options.hpp>

#include "common/devcash_context.h"
#include "common/logger.h"

namespace Devcash {

enum eDebugMode {off, on, toy};

struct devcash_options {
  std::string bind_endpoint;
  std::vector<std::string> host_vector{};
  eAppMode mode;
  unsigned int node_index;
  unsigned int shard_index;
  unsigned int num_consensus_threads;
  unsigned int num_validator_threads;
  std::string scan_file;
  std::string write_file;
  std::string trace_file;
  std::string inn_keys;
  std::string node_keys;
  std::string wallet_keys;
  unsigned int generate_count;
  unsigned int tx_batch_size;
  eDebugMode debug_mode;
};

std::unique_ptr<struct devcash_options> parse_options(int argc, char** argv) {

  namespace po = boost::program_options;

  std::unique_ptr<devcash_options> options(new devcash_options());

  try {
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
      ("debug-mode", po::value<std::string>(), "Debug mode (on|toy|perf) for testing")
      ("mode", po::value<std::string>(), "Devcash mode (T1|T2|scan)")
      ("node-index", po::value<unsigned int>(), "Index of this node")
      ("shard-index", po::value<unsigned int>(), "Index of this shard")
      ("num-consensus-threads", po::value<unsigned int>(), "Number of consensus threads")
      ("num-validator-threads", po::value<unsigned int>(), "Number of validation threads")
      ("host-list,host", po::value<std::vector<std::string>>(),
       "Client URI (i.e. tcp://192.168.10.1:5005). Option can be repeated to connect to multiple nodes.")
      ("bind-endpoint", po::value<std::string>(), "Endpoint for server (i.e. tcp://*:5556)")
      ("scan-file", po::value<std::string>(), "Initial transaction or blockchain input file")
      ("output", po::value<std::string>(), "Blockchain output path in binary JSON or CBOR")
      ("trace-output", po::value<std::string>(), "Output path to JSON trace file (Chrome)")
      ("inn-keys", po::value<std::string>(), "Path to INN key file")
      ("node-keys", po::value<std::string>(), "Path to Node key file")
      ("wallet-keys", po::value<std::string>(), "Path to Wallet key file")
      ("generate-tx", po::value<unsigned int>(), "Generate at least this many Transactions")
      ("tx-batch-size", po::value<unsigned int>(), "Target size of transaction batches")
      ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << desc << "\n";
      return nullptr;
    }

    if (vm.count("mode")) {
      std::string mode = vm["mode"].as<std::string>();
      if (mode == "SCAN") {
        options->mode = scan;
      } else if (mode == "T1") {
        options->mode = T1;
      } else if (mode == "T2") {
        options->mode = T2;
      }
      LOG_INFO << "mode: " << options->mode;
    } else {
      LOG_INFO << "mode was not set.";
    }

    if (vm.count("debug-mode")) {
      std::string debug_mode = vm["debug-mode"].as<std::string>();
      if (debug_mode == "on") {
        options->debug_mode = on;
      } else if (debug_mode == "toy") {
        options->debug_mode = toy;
      } else {
        options->debug_mode = off;
      }
      LOG_INFO << "debug_mode: " << options->debug_mode;
    } else {
      LOG_INFO << "debug_mode was not set.";
    }

    if (vm.count("node-index")) {
      options->node_index = vm["node-index"].as<unsigned int>();
      LOG_INFO << "Node index: " << options->node_index;
    } else {
      LOG_INFO << "Node index was not set.";
    }

    if (vm.count("shard-index")) {
      options->shard_index = vm["shard-index"].as<unsigned int>();
      LOG_INFO << "Shard index: " << options->shard_index;
    } else {
      LOG_INFO << "Shard index was not set.";
    }

    if (vm.count("num-consensus-threads")) {
      options->num_consensus_threads = vm["num-consensus-threads"].as<unsigned int>();
      LOG_INFO << "Num consensus threads: " << options->num_consensus_threads;
    } else {
      LOG_INFO << "Num consensus threads was not set, defaulting to 10";
      options->num_consensus_threads = 10;
    }

    if (vm.count("num-validator-threads")) {
      options->num_validator_threads = vm["num-validator-threads"].as<unsigned int>();
      LOG_INFO << "Num validator threads: " << options->num_validator_threads;
    } else {
      LOG_INFO << "Num validator threads was not set, defaulting to 10";
      options->num_validator_threads = 10;
    }

    if (vm.count("bind-endpoint")) {
      options->bind_endpoint = vm["bind-endpoint"].as<std::string>();
      LOG_INFO << "Bind URI: " << options->bind_endpoint;
    } else {
      LOG_INFO << "Bind URI was not set";
    }

    if (vm.count("host-list")) {
      options->host_vector = vm["host-list"].as<std::vector<std::string>>();
      LOG_INFO << "Node URIs:";
      for (auto i : options->host_vector) {
        LOG_INFO << "  " << i;
      }
    }

    if (vm.count("scan-file")) {
      options->scan_file = vm["scan-file"].as<std::string>();
      LOG_INFO << "Scan file: " << options->scan_file;
    } else {
      LOG_INFO << "Scan file was not set.";
    }

    if (vm.count("output")) {
      options->write_file = vm["output"].as<std::string>();
      LOG_INFO << "Output file: " << options->write_file;
    } else {
      LOG_INFO << "Output file was not set.";
    }

    if (vm.count("trace-output")) {
      options->trace_file = vm["trace-output"].as<std::string>();
      LOG_INFO << "Trace output file: " << options->trace_file;
    } else {
      LOG_INFO << "Trace file was not set.";
    }

    if (vm.count("inn-keys")) {
      options->inn_keys = vm["inn-keys"].as<std::string>();
      LOG_INFO << "INN keys file: " << options->trace_file;
    } else {
      LOG_INFO << "INN keys file was not set.";
    }

    if (vm.count("node-keys")) {
      options->node_keys = vm["node-keys"].as<std::string>();
      LOG_INFO << "Node keys file: " << options->node_keys;
    } else {
      LOG_INFO << "Node keys file was not set.";
    }

    if (vm.count("wallet-keys")) {
      options->wallet_keys = vm["wallet-keys"].as<std::string>();
      LOG_INFO << "Wallet keys file: " << options->wallet_keys;
    } else {
      LOG_INFO << "Wallet keys file was not set.";
    }

    if (vm.count("generate-tx")) {
      options->generate_count = vm["generate-tx"].as<unsigned int>();
      LOG_INFO << "Generate Transactions: " << options->generate_count;
    } else {
      LOG_INFO << "Generate Transactions was not set, defaulting to 0";
      options->generate_count = 0;
    }

    if (vm.count("tx-batch-size")) {
      options->tx_batch_size = vm["tx-batch-size"].as<unsigned int>();
      LOG_INFO << "Transaction batch size: " << options->tx_batch_size;
    } else {
      LOG_INFO << "Transaction batch size was not set, defaulting to 10";
      options->tx_batch_size = 10;
    }

  }
  catch(std::exception& e) {
    LOG_ERROR << "error: " << e.what();
    return nullptr;
  }

  return options;
}

} // namespace Devcash

#endif /* COMMON_ARGUMENT_PARSER_H_ */
