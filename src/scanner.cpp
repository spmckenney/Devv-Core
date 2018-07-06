
/*
 * scanner.cpp the main class for a block scanner.
 * @note the mode must be set appropriately
 * Use T1 to scan a tier 1 chain, T2 to scan a T2 chain, and scan to scan raw transactions.
 *
 *  Created on: May 20, 2018
 *  Author: Nick Williams
 */

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <functional>
#include <thread>
#include <boost/filesystem.hpp>

#include "common/argument_parser.h"
#include "common/devcash_context.h"
#include "modules/BlockchainModule.h"
#include "primitives/json_interface.h"
#include "primitives/block_tools.h"

using namespace Devcash;
namespace fs = boost::filesystem;

struct scanner_options {
  eAppMode mode  = eAppMode::T1;
  unsigned int node_index = 0;
  unsigned int shard_index = 0;
  std::string working_dir;
  std::string write_file;
  std::string inn_keys;
  std::string node_keys;
  std::string wallet_keys;
  unsigned int generate_count;
  unsigned int tx_limit;
  eDebugMode debug_mode;
};

std::unique_ptr<struct scanner_options> ParseScannerOptions(int argc, char** argv);

int main(int argc, char* argv[])
{
  init_log();

  CASH_TRY {
    auto options = ParseScannerOptions(argc, argv);

    if (!options) {
      LOG_ERROR << "ParseScannerOptions failed";
      exit(-1);
    }

    size_t block_counter = 0;
    size_t tx_counter = 0;
    size_t tfer_count = 0;

    DevcashContext this_context(options->node_index
      , options->shard_index
      , options->mode
      , options->inn_keys
      , options->node_keys
      , options->wallet_keys);

    KeyRing keys(this_context);

    fs::path p(options->working_dir);
    if (p.empty()) {
      LOG_WARNING << "Invalid path: "+options->working_dir;
      return(false);
    }

    if (!is_directory(p)) {
      LOG_ERROR << "Error opening dir: " << options->working_dir << " is not a directory";
      return false;
    }

    std::string out;
    std::vector<std::string> files;
    for(auto& entry : boost::make_iterator_range(fs::directory_iterator(p), {})) {
      files.push_back(entry.path().string());
    }
    for  (auto const& file_name : files) {
      LOG_DEBUG << "Reading " << file_name;
      std::ifstream file(file_name, std::ios::binary);
      file.unsetf(std::ios::skipws);
      std::size_t file_size;
      file.seekg(0, std::ios::end);
      file_size = file.tellg();
      file.seekg(0, std::ios::beg);

      std::vector<byte> raw;
      raw.reserve(file_size);
      raw.insert(raw.begin(), std::istream_iterator<byte>(file), std::istream_iterator<byte>());
      bool is_block = IsBlockData(raw);
      bool is_transaction = IsTxData(raw);
      if (is_block) { LOG_INFO << file_name << " has blocks."; }
      if (is_transaction) { LOG_INFO << file_name << " has transactions."; }
      if (!is_block && !is_transaction) {
        LOG_WARNING << file_name << " contains unknown data.";
      }
      size_t file_blocks = 0;
      size_t file_txs = 0;
      size_t file_tfer = 0;

      ChainState priori;

      InputBuffer buffer(raw);
      while (buffer.getOffset() < static_cast<size_t>(file_size)) {
        if (is_transaction) {
          Tier2Transaction tx(Tier2Transaction::Create(buffer, keys, true));
          file_txs++;
          file_tfer += tx.getTransfers().size();
          out += tx.getJSON();
        } else if (is_block) {
          size_t span = buffer.getOffset();
          FinalBlock one_block(buffer, priori, keys, options->mode);
          if (buffer.getOffset() == span) {
            LOG_WARNING << file_name << " has invalid block!";
            break;
		  }
          size_t txs = one_block.getNumTransactions();
          size_t tfers = one_block.getNumTransfers();
          priori = one_block.getChainState();
          out += GetJSON(one_block);

          LOG_INFO << std::to_string(txs)+" txs, transfers: "+std::to_string(tfers);
          file_blocks++;
          file_txs += txs;
          file_tfer += tfers;
        } else {
          LOG_WARNING << "!is_block && !is_transaction";
        }
      }

      bool first_addr = true;
      std::stringstream state_stream;
      if (priori.getStateMap().empty()) { LOG_INFO << file_name << " END with no state"; }
      for (auto const& item : priori.getStateMap()) {
        LOG_INFO << file_name << " END STATE BEGIN: ";
        state_stream << "{\"Addr\":\"";
        state_stream << ToHex(std::vector<byte>(std::begin(item.first)
          , std::end(item.first)));
        state_stream << "\",\"state\":[";
        bool first_coin = true;
        for (auto coin = item.second.begin(); coin != item.second.end(); ++coin) {
          state_stream << "\""+std::to_string(coin->first)+":"+std::to_string(coin->second);
          if (first_coin) {
            first_coin = false;
          } else {
            state_stream << ",";
          }
        }
        state_stream << "]";
        if (first_addr) {
          first_addr = false;
        } else {
          state_stream << ",";
        }
        LOG_INFO << state_stream.str();
        LOG_INFO << file_name << " END STATE END";
      }

      LOG_INFO << file_name << " has " << std::to_string(file_txs)
              << " txs, " +std::to_string(file_tfer)+" tfers in "+std::to_string(file_blocks)+" blocks.";
      block_counter += file_blocks;
      tx_counter += file_txs;
      tfer_count += file_tfer;
    }
    LOG_INFO << "Dir has "+std::to_string(tx_counter)+" txs, "+std::to_string(tfer_count)+" tfers in "+std::to_string(block_counter)+" blocks.";

    if (!options->write_file.empty()) {
      std::ofstream out_file(options->write_file, std::ios::out);
      if (out_file.is_open()) {
        out_file << out;
        out_file.close();
      } else {
        LOG_FATAL << "Failed to open output file '" << options->write_file << "'.";
        return(false);
      }
    }

    return(true);
  } CASH_CATCH (...) {
    std::exception_ptr p = std::current_exception();
    std::string err("");
    err += (p ? p.__cxa_exception_type()->name() : "null");
    LOG_FATAL << "Error: "+err <<  std::endl;
    std::cerr << err << std::endl;
    return(false);
  }
}

std::unique_ptr<struct scanner_options> ParseScannerOptions(int argc, char** argv) {

  namespace po = boost::program_options;

  auto options = std::make_unique<scanner_options>();

  try {
    po::options_description desc("\n\
" + std::string(argv[0]) + " [OPTIONS] \n\
\n\
A block scanner.\n\
Use T1 to scan a tier 1 chain, T2 to scan a T2 chain, and scan to\n\
scan raw transactions.\n\
\n\
Required parameters");
    desc.add_options()
        ("mode", po::value<std::string>(), "Devcash mode (T1|T2|scan)")
        ("node-index", po::value<unsigned int>(), "Index of this node")
        ("shard-index", po::value<unsigned int>(), "Index of this shard")
        ("working-dir", po::value<std::string>(), "Directory where inputs are read and outputs are written")
        ("output", po::value<std::string>(), "Output path in binary JSON or CBOR")
        ("inn-keys", po::value<std::string>(), "Path to INN key file")
        ("node-keys", po::value<std::string>(), "Path to Node key file")
        ("wallet-keys", po::value<std::string>(), "Path to Wallet key file")
        ("generate-tx", po::value<unsigned int>(), "Generate at least this many Transactions")
        ("tx-limit", po::value<unsigned int>(), "Number of transaction to process before shutting down.")
        ;

    po::options_description d2("Optional parameters");
    d2.add_options()
        ("help", "produce help message")
        ("debug-mode", po::value<std::string>(), "Debug mode (on|off|perf) for testing")
        ;
    desc.add(d2);

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
      } else if (debug_mode == "perf") {
        options->debug_mode = perf;
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

    if (vm.count("working-dir")) {
      options->working_dir = vm["working-dir"].as<std::string>();
      LOG_INFO << "Working dir: " << options->working_dir;
    } else {
      LOG_INFO << "Working dir was not set.";
    }

    if (vm.count("output")) {
      options->write_file = vm["output"].as<std::string>();
      LOG_INFO << "Output file: " << options->write_file;
    } else {
      LOG_INFO << "Output file was not set.";
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

    if (vm.count("tx-limit")) {
      options->tx_limit = vm["tx-limit"].as<unsigned int>();
      LOG_INFO << "Transaction limit: " << options->tx_limit;
    } else {
      LOG_INFO << "Transaction limit was not set, defaulting to 0 (unlimited)";
      options->tx_limit = 100;
    }
  }
  catch (std::exception& e) {
    LOG_ERROR << "error: " << e.what();
    return nullptr;
  }

  return options;
}
