#pragma once

#ifndef COMMON_ARGUMENT_PARSER_H_
#define COMMON_ARGUMENT_PARSER_H_

#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <exception>
#include <algorithm>

#include <boost/program_options.hpp>

#include "devcash_context.h"
#include "logger.h"

namespace Devcash {

template <typename T>
void RemoveDuplicates(std::vector<T>& vec)
{
  std::sort(vec.begin(), vec.end());
  vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
}

struct devcash_options {
  std::string bind_endpoint;
  std::vector<std::string> host_vector{};
  eAppMode mode = T1;
  unsigned int node_index = 0;
  unsigned int shard_index = 0;
  unsigned int num_consensus_threads = 1;
  unsigned int num_validator_threads = 1;
  std::string working_dir;
  std::string trace_file;
  std::string inn_keys;
  std::string node_keys;
  std::string key_pass;
  std::string stop_file;
  eDebugMode debug_mode = off;
};

std::unique_ptr<struct devcash_options> ParseDevcashOptions(int argc, char** argv) {

  namespace po = boost::program_options;

  std::unique_ptr<devcash_options> options(new devcash_options());
  std::vector<std::string> config_filenames;

  try {
    po::options_description general("General Options");
    general.add_options()
        ("help,h", "produce help message")
        ("version,v", "print version string")
        ("config", po::value(&config_filenames), "Config file where options may be specified (can be specified more than once)")
        ;

    po::options_description behavior("Identity and Behavior Options");
    behavior.add_options()
        ("mode", po::value<std::string>(), "Devcash mode (T1|T2|scan)")
        ("node-index", po::value<unsigned int>(), "Index of this node")
        ("shard-index", po::value<unsigned int>(), "Index of this shard")
        ("host-list,H", po::value<std::vector<std::string>>()->composing(),
         "Client URI (i.e. tcp://192.168.10.1:5005). Option can be repeated to connect to multiple nodes.")
        ("bind-endpoint", po::value<std::string>(), "Endpoint for server (i.e. tcp://*:5556)")
        ("inn-keys", po::value<std::string>(), "Path to INN key file")
        ("node-keys", po::value<std::string>(), "Path to Node key file")
        ("key-pass", po::value<std::string>(), "Password for private keys")
        ;

    po::options_description debugging("Debugging and Performance Options");
    debugging.add_options()
        ("debug-mode", po::value<std::string>(), "Debug mode (on|toy|perf) for testing")
        ("trace-output", po::value<std::string>(), "Output path to JSON trace file (Chrome)")
        ("num-consensus-threads", po::value<unsigned int>(), "Number of consensus threads")
        ("num-validator-threads", po::value<unsigned int>(), "Number of validation threads")
        ("working-dir", po::value<std::string>(), "Directory where inputs are read and outputs are written")
        ("stop-file", po::value<std::string>(), "A file in working-dir indicating that this node should stop.")
        ;

    po::options_description all_options;
    all_options.add(general);
    all_options.add(behavior);
    all_options.add(debugging);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
                  options(all_options).
                  run(),
              vm);

    if (vm.count("help")) {
      LOG_INFO << all_options;
      return nullptr;
    }

    if(vm.count("config") > 0)
    {
      config_filenames = vm["config"].as<std::vector<std::string> >();

      for(size_t i = 0; i < config_filenames.size(); ++i)
      {
        std::ifstream ifs(config_filenames[i].c_str());
        if(ifs.fail())
        {
          LOG_ERROR << "Error opening config file: " << config_filenames[i];
          return nullptr;
        }
        po::store(po::parse_config_file(ifs, all_options), vm);
      }
    }

    po::store(po::parse_command_line(argc, argv, all_options), vm);
    po::notify(vm);

    if (vm.count("mode")) {
      std::string mode = vm["mode"].as<std::string>();
      if (mode == "SCAN") {
        options->mode = scan;
      } else if (mode == "T1") {
        options->mode = T1;
      } else if (mode == "T2") {
        options->mode = T2;
      } else {
        LOG_WARNING << "unknown mode: " << mode;
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
      RemoveDuplicates(options->host_vector);
      LOG_INFO << "Node URIs:";
      for (auto i : options->host_vector) {
        LOG_INFO << "  " << i;
      }
    }

    if (vm.count("working-dir")) {
      options->working_dir = vm["working-dir"].as<std::string>();
      LOG_INFO << "Working dir: " << options->working_dir;
    } else {
      LOG_INFO << "Working dir was not set.";
    }

    if (vm.count("trace-output")) {
      options->trace_file = vm["trace-output"].as<std::string>();
      LOG_INFO << "Trace output file: " << options->trace_file;
    } else {
      LOG_INFO << "Trace file was not set.";
    }

    if (vm.count("inn-keys")) {
      options->inn_keys = vm["inn-keys"].as<std::string>();
      LOG_INFO << "INN keys file: " << options->inn_keys;
    } else {
      LOG_INFO << "INN keys file was not set.";
    }

    if (vm.count("node-keys")) {
      options->node_keys = vm["node-keys"].as<std::string>();
      LOG_INFO << "Node keys file: " << options->node_keys;
    } else {
      LOG_INFO << "Node keys file was not set.";
    }

    if (vm.count("key-pass")) {
      options->key_pass = vm["key-pass"].as<std::string>();
      LOG_INFO << "Key pass: " << options->key_pass;
    } else {
      LOG_INFO << "Key pass was not set.";
    }

    if (vm.count("stop-file")) {
      options->stop_file = vm["stop-file"].as<std::string>();
      LOG_INFO << "Stop file: " << options->stop_file;
    } else {
      LOG_INFO << "Stop file was not set. Use a signal to stop the node.";
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
