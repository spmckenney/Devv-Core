/*
 * scanner.cpp the main class for a block scanner.
 * @note the mode must be set appropriately
 * Use T1 to scan a tier 1 chain, T2 to scan a T2 chain, and scan to scan raw transactions.
 *
 * Scans and checks public keys without using any private keys.
 * Handles each file independently, so blockchains should be in a single file.
 *
 *  Created on: May 20, 2018
 *  Author: Nick Williams
 */

#include <string>
#include <iostream>
#include <fstream>
#include <functional>
#include <thread>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "common/devcash_context.h"
#include "modules/BlockchainModule.h"
#include "primitives/json_interface.h"
#include "primitives/block_tools.h"

using namespace Devcash;
namespace fs = boost::filesystem;

struct scanner_options {
  eAppMode mode  = eAppMode::T1;
  std::string working_dir;
  std::string write_file;
  eDebugMode debug_mode;
  unsigned int version;
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
    size_t total_volume = 0;

    KeyRing keys;

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
      uint64_t start_time = 0;
      uint64_t blocktime = 0;
      uint64_t volume = 0;

      ChainState priori;
      ChainState posteri;

      InputBuffer buffer(raw);
      while (buffer.getOffset() < static_cast<size_t>(file_size)) {
        try {
          if (is_transaction) {
            Tier2Transaction tx(Tier2Transaction::Create(buffer, keys, true));
            file_txs++;
            file_tfer += tx.getTransfers().size();
            out += tx.getJSON();
          } else if (is_block) {
            FinalBlock one_block(buffer, priori, keys, options->mode);

            if (one_block.getVersion() != options->version) {
              LOG_WARNING << "Unexpected block version ("+std::to_string(one_block.getVersion())+") in " << file_name;
            }
            size_t txs_count = one_block.getNumTransactions();
            size_t tfers = one_block.getNumTransfers();
            uint64_t previous_time = blocktime;
            blocktime = one_block.getBlockTime();
            uint64_t duration = blocktime-previous_time;
            priori = one_block.getChainState();
            out += GetJSON(one_block);

            out += std::to_string(txs_count)+" txs, transfers: "+std::to_string(tfers)+"\n";
            LOG_INFO << std::to_string(txs_count)+" txs, transfers: "+std::to_string(tfers);
            out += "Duration: "+std::to_string(duration)+" ms.\n";
            LOG_INFO << "Duration: "+std::to_string(duration)+" ms.";
            if (duration != 0 && previous_time != 0) {
              out += "Rate: "+std::to_string(txs_count*1000/duration)+" txs/sec\n";
              LOG_INFO << "Rate: "+std::to_string(txs_count*1000/duration)+" txs/sec";
            } else if (previous_time == 0) {
              start_time = blocktime;
            }
            uint64_t block_volume = one_block.getVolume();
            volume += block_volume;
            out += "Volume: "+std::to_string(block_volume)+"\n";
            LOG_INFO << "Volume: "+std::to_string(block_volume);

            Summary block_summary(Summary::Create());
            std::vector<TransactionPtr> txs = one_block.CopyTransactions();
            for (TransactionPtr& item : txs) {
              if (!item->isValid(posteri, keys, block_summary)) {
                LOG_WARNING << "A transaction is invalid. TX details: ";
                LOG_WARNING << item->getJSON();
              }
            }
            if (block_summary.getCanonical() != one_block.getSummary().getCanonical()) {
              LOG_WARNING << "A final block summary is invalid. Summary datails: ";
              LOG_WARNING << GetJSON(one_block.getSummary());
              LOG_WARNING << "Transaction details: ";
              for (TransactionPtr& item : txs) {
                LOG_WARNING << item->getJSON();
              }
            }

            file_blocks++;
            file_txs += txs_count;
            file_tfer += tfers;
          } else {
            LOG_WARNING << "!is_block && !is_transaction";
          }
        } catch (const std::exception& e) {
          LOG_ERROR << "Error scanning " << file_name << " skipping to next file.  Error details: "+FormatException(&e, "scanner");
          LOG_ERROR << "Offset/Size: "+std::to_string(buffer.getOffset())+"/"+std::to_string(file_size);
          break;
        }
      }

      if (posteri.getStateMap().empty()) {
        out += "End with no chainstate.";
	  } else {
        out += "End chainstate: " + WriteChainStateMap(posteri.getStateMap());
	  }


      out += file_name + " has " + std::to_string(file_txs)
              + " txs, " +std::to_string(file_tfer)+" tfers in "+std::to_string(file_blocks)+" blocks.\n";
      out += file_name + " coin volume is "+std::to_string(volume)+"\n";
      uint64_t duration = blocktime-start_time;
      if (duration != 0 && start_time != 0) {
        out += file_name+" overall rate: "+std::to_string(file_txs*1000/duration)+" txs/sec\n";
        LOG_INFO << file_name << " overall rate: "+std::to_string(file_txs*1000/duration)+" txs/sec";
      }
      LOG_INFO << file_name << " has " << std::to_string(file_txs)
              << " txs, " +std::to_string(file_tfer)+" tfers in "+std::to_string(file_blocks)+" blocks.";
      LOG_INFO << file_name + " coin volume is "+std::to_string(volume);

      block_counter += file_blocks;
      tx_counter += file_txs;
      tfer_count += file_tfer;
      total_volume += volume;
    }
    out += "Dir has "+std::to_string(tx_counter)+" txs, "
      +std::to_string(tfer_count)+" tfers in "+std::to_string(block_counter)+" blocks.";
    out += "Grand total coin volume is "+std::to_string(total_volume)+"\n";
    LOG_INFO << "Dir has "+std::to_string(tx_counter)+" txs, "
      +std::to_string(tfer_count)+" tfers in "+std::to_string(block_counter)+" blocks.";
    LOG_INFO << "Grand total coin volume is "+std::to_string(total_volume);

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
        ("working-dir", po::value<std::string>(), "Directory where inputs are read and outputs are written")
        ("output", po::value<std::string>(), "Output path in binary JSON or CBOR")
        ;

    po::options_description d2("Optional parameters");
    d2.add_options()
        ("expect-version", "look for this block version while scanning")
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

    if (vm.count("expect-version")) {
      options->version = vm["expect-version"].as<unsigned int>();
      LOG_INFO << "Expect Block Version: " << options->version;
    } else {
      LOG_INFO << "Expect Block Version was not set, defaulting to 0";
      options->version = 0;
    }
  }
  catch (std::exception& e) {
    LOG_ERROR << "error: " << e.what();
    return nullptr;
  }

  return options;
}
