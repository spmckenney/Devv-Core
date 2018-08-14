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
#include "io/file_ops.h"
#include "consensus/KeyRing.h"
#include "pbuf/devv_pbuf.h"

using namespace Devcash;
namespace fs = boost::filesystem;

/**
 * Holds command-line options
 */
struct devvsign_options {
  enum eTxFileType {
    PROTOBUF = 0,
    JSON = 1,
    XML = 2,
    UNKNOWN = 3
  };

  fs::path tx_file = "";
  fs::path key_file = "";
  std::string key_pass;
  eTxFileType file_type = eTxFileType::UNKNOWN;
};

/**
 * Parse command-line options
 * @param argc
 * @param argv
 * @return
 */
std::unique_ptr<struct devvsign_options> ParseDevvsignOptions(int argc, char** argv);

void TestSign(EC_KEY& ec_key) {
  std::vector<byte> msg = {'h', 'e', 'l', 'l', 'o'};
  Hash test_hash;
  test_hash = DevcashHash(msg);

  Signature sig = SignBinary(&ec_key, test_hash);
  if (!VerifyByteSig(&ec_key, test_hash, sig)) {
    throw std::runtime_error("VerifyByteSig failed");
  }
}

int main(int argc, char* argv[]) {

  std::unique_ptr<struct devvsign_options> options;
  try {
    options = ParseDevvsignOptions(argc, argv);

    if (!options) {
      exit(-1);
    }
  } catch (std::exception& e) {
    std::cerr << "Error parsing options: " << e.what();
    exit(-1);
  }

  auto env = ReadBinaryFile(options->tx_file);

  std::string env_str(env.begin(), env.end());


  auto tup = ReadKeyFile(options->key_file);
  std::cout << tup.address;
  std::cout << tup.key;

  options->file_type = devvsign_options::eTxFileType::PROTOBUF;

  auto ec_key = LoadEcKey(tup.address, tup.key, options->key_pass);

  if (ec_key == nullptr) {
    throw std::runtime_error("LoadEcKey failed");
  }
  try {
    TestSign(*ec_key);
  } catch (std::runtime_error& rte) {
    std::cerr << "TestSign failed";
  }

  KeyRing keys;
  keys.addNodeKeyPair(tup.address, tup.key, options->key_pass);


  std::cout << "made it: " << tup.address.size() << std::endl;

  Devv::proto::Envelope envelope;
  envelope.ParseFromString(env_str);

  auto pb_transactions = envelope.txs();
  std::cout << "num tx: " << pb_transactions.size();
  for (auto const& transaction : pb_transactions) {
    auto tx = CreateTransaction(transaction, keys, true);
    std::cout << "Sig: " << tx->getSignature().getJSON();
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
      ("tx-file,f", po::value<fs::path>(), "Transaction file")
      ("private-key,k", po::value<fs::path>(), "File containing private key to use in signing")
      ("key-pass", po::value<std::string>(), "Password for private keys")
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

  if (vm.count("tx-file")) {
    options->tx_file = vm["tx-file"].as<fs::path>();
  } else {
    throw std::runtime_error("A transaction file must be specified.");
  }

  if (vm.count("private-key")) {
    options->key_file = vm["private-key"].as<fs::path>();
  } else {
    throw std::runtime_error("Private key file was not set");
  }

  if (vm.count("key-pass")) {
    options->key_pass = vm["key-pass"].as<std::string>();
  } else {
    throw std::runtime_error("Key pass was not set.");
  }

  return options;
}
