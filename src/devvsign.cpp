/*
 * devvsign.cpp signs a transaction using a given private key
 *
 *  Created on: 8/8/18
 *      Author: Shawn McKenney
 */

#include <cstdio>
#include <iostream>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "common/devcash_context.h"

using namespace Devcash;
namespace fs = boost::filesystem;

/**
 * Holds command-line options
 */
struct devvsign_options {
  std::string json_tx = "";
  fs::path key_file = "";
};

/**
 * Parse command-line options
 * @param argc
 * @param argv
 * @return
 */
std::unique_ptr<struct devvsign_options> ParseDevvsignOptions(int argc, char** argv);

int main(int argc, char* argv[]) {

  try {
    auto options = ParseDevvsignOptions(argc, argv);

    if (!options) {
      exit(-1);
    }
  } catch (std::exception& e) {
    std::cerr << "Error parsing options: " << e.what();
  }

}

std::unique_ptr<struct devvsign_options> ParseDevvsignOptions(int argc, char** argv) {

  namespace po = boost::program_options;

  std::unique_ptr<devvsign_options> options(new devvsign_options());
  std::vector<std::string> config_filenames;

  po::options_description general("General Options\n\
" + std::string(argv[0]) + " [OPTIONS] \n\
\n\
Accepts a devv transaction as a json string and a file containing \n\
a private key. devvsign will sign the key and upon success, return\n\
a value of 0 and print the signature in hex format to stdout\n\
\nAllowed options");
  general.add_options()
      ("help,h", "produce help message")
      ("version,v", "print version string")
      ("config", po::value(&config_filenames), "Config file where options may be specified (can be specified more than once)")
      ("json-tx,t", po::value<std::string>(), "Transaction in JSON format")
      ("private-key,k", po::value<fs::path>(), "File containing private key to use in signing")
      ;

  po::options_description all_options;
  all_options.add(general);

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).
                options(all_options).
                run(),
                vm);

  if (vm.count("help")) {
    std::cout << all_options;
    return nullptr;
  }

  if (vm.count("json-tx")) {
    options->json_tx = vm["json-tx"].as<std::string>();
  } else {
    throw std::runtime_error("A JSON transaction must be provided");
  }

  if (vm.count("private-key")) {
    options->key_file = vm["private-key"].as<fs::path>();
  } else {
    throw std::runtime_error("Private key file was not set");
  }

  return options;
}
