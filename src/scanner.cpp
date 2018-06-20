
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
#include "node/DevcashNode.h"
#include "primitives/json_interface.h"

using namespace Devcash;
namespace fs = boost::filesystem;

#define UNUSED(x) ((void)x)

int main(int argc, char* argv[])
{

  init_log();

  CASH_TRY {
    std::unique_ptr<devcash_options> options = parse_options(argc, argv);

    if (!options) {
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
      , options->wallet_keys
      , options->sync_port
      , options->sync_host);

    KeyRing keys(this_context);

    fs::path p(options->working_dir);
    if (p.empty()) {
      LOG_WARNING << "Invalid path: "+options->working_dir;
      return(false);
    }

    std::string out;
    for(auto& entry : boost::make_iterator_range(fs::directory_iterator(p), {})) {
      LOG_DEBUG << "Reading " << entry;
      std::ifstream file(entry.path().string(), std::ios::binary);
      file.unsetf(std::ios::skipws);
      std::size_t file_size;
      file.seekg(0, std::ios::end);
      file_size = file.tellg();
      file.seekg(0, std::ios::beg);

      std::vector<byte> raw;
      raw.reserve(file_size);
      raw.insert(raw.begin(), std::istream_iterator<byte>(file), std::istream_iterator<byte>());
      size_t file_blocks = 0;
      size_t file_txs = 0;
      size_t file_tfer = 0;

      ChainState priori;

      InputBuffer buffer(raw);
      while (buffer.getOffset() < file_size) {
        if (options->mode == eAppMode::scan) {
          Tier2Transaction tx(buffer, keys, true);
          file_txs++;
          file_tfer += tx.getTransfers().size();
          out += tx.getJSON();
        } else {
          size_t span = buffer.getOffset();
          FinalBlock one_block(buffer, priori, keys, options->mode);
          if (buffer.getOffset() == span) {
            LOG_WARNING << entry << " has invalid block!";
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
        }
      }

      bool first_addr = true;
      std::stringstream state_stream;
      if (priori.getStateMap().empty()) LOG_INFO << entry << "END with no state";
      for (auto const& item : priori.getStateMap()) {
        LOG_INFO << entry << " END STATE BEGIN: ";
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
        LOG_INFO << entry << " END STATE END";
      }

      LOG_INFO << entry << " has "+std::to_string(file_txs)+" txs, "+std::to_string(file_tfer)+" tfers in "+std::to_string(file_blocks)+" blocks.";
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
