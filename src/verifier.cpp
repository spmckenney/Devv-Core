
/*
 * verifier.cpp scans a directory for transactions and blocks.
 * Then, it accumulates the states implied by all the sound transactions it finds
 * as well as getting the chain state implied by the blockchain(s) in the directory.
 * Finally, it compares these states and dumps them if they are not equal.
 *
 * If states are not equal, keep in mind that order and timing matter.
 * A transaction may be sound, but not valid when it is announced to the verifiers.
 * In this case, this transaction will be recorded in the chainstate implied by sound transactions,
 * but not the chainstate created in the actual chains and the chains may be correct even though
 * they are missing that transaction, which could not be processed when it was announced.
 *
 * Likewise, this verion is only designed to verify 1 chain.
 * If there are transactions being distributed among multiple chains, the chains may arrive at
 * different states from the one implied by the set of transactions themselves.
 *
 *  Created on: May 31, 2018
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
#include "io/message_service.h"

using namespace Devcash;

namespace fs = boost::filesystem;

#define UNUSED(x) ((void)x)

/** Checks if binary is encoding a block
 * @note this function is pretty heuristic, do not use in critical cases
 * @return true if this data encodes a block
 * @return false otherwise
 */
bool isBlockData(std::vector<byte> raw) {
  //check if big enough
  if (raw.size() < FinalBlock::MinSize()) return false;
  //check version
  if (raw[0] != 0x00) return false;
  size_t offset = 9;
  uint64_t block_time = BinToUint64(raw, offset);
  //check blocktime is from 2018 or newer.
  if (block_time < 1514764800) return false;
  //check blocktime is in past
  if (block_time > getEpoch()) return false;
  return true;
}

/** Checks if binary is encoding Transactions
 * @note this function is pretty heuristic, do not use in critical cases
 * @return true if this data encodes Transactions
 * @return false otherwise
 */
bool isTxData(std::vector<byte> raw) {
  //check if big enough
  if (raw.size() < Transaction::MinSize()) return false;
  //check transfer count
  uint64_t xfer_count = BinToUint64(raw, 0);
  size_t tx_size = Transaction::MinSize()+(Transfer::Size()*xfer_count);
  if (raw.size() < tx_size) return false;
  //check operation
  if (raw[8] >= 4) return false;
  return true;
}

/** Checks if two chain state maps contain the same state
 * @return true if the maps have the same state
 * @return false otherwise
 */
bool CompareChainStateMaps(
    std::map<Address, std::map<uint64_t, uint64_t>> first
    , std::map<Address, std::map<uint64_t, uint64_t>> second) {
  if (first.size() != second.size()) return false;
  for (auto i=first.begin(), j = second.begin(); i != first.end(); ++i, ++j) {
    if (i->first != j->first) return false;
    if (i->second.size() != j->second.size()) return false;
    for (auto x=i->second.begin(), y=j->second.begin(); x != i->second.end();
        ++x, ++y) {
      if (x->first != y->first) return false;
      if (x->second != y->second) return false;
    }
  }
  return true;
}

/** Dumps the map inside a chainstate object into a human-readable JSON format.
 * @return a string containing a description of the chain state.
 */
std::string WriteChainStateMap(std::map<Address, std::map<uint64_t, uint64_t>> map) {
  std::string out("{");
  bool first_addr = true;
  for (auto e : map) {
    if (first_addr) {
      first_addr = false;
    } else {
      out += ",";
    }
    Address a = e.first();
    out += "\""+toHex(std::vector<byte>(std::begin(a), std::end(a)))+"\":[";
    bool is_first;
    for (auto f : e.second) {
      if (is_first) {
        is_first = false;
      } else {
        out += ",";
      }
      out += std::to_string(f.first)+":"+std::to_string(f.second);
    }
    out += "]";
  }
  return out;
}

int main(int argc, char* argv[])
{

  init_log();

  CASH_TRY {
    std::unique_ptr<devcash_options> options = parse_options(argc, argv);

    if (!options) {
      exit(-1);
    }

    zmq::context_t context(1);

    DevcashContext this_context(options->node_index
                                , options->shard_index
                                , options->mode
                                , options->inn_keys
                                , options->node_keys
                                , options->wallet_keys
                                , options->sync_port
                                , options->sync_host);
    KeyRing keys(this_context);

    LOG_DEBUG << "Scanning " << options->scan_dir;
    MTR_SCOPE_FUNC();

    ChainState priori;
    ChainState posteri;
    Summary summary;

    fs::path p(options->scan_dir);

    if (!is_directory(p)) {
      LOG_ERROR << "Error opening dir: " << options->scan_dir << " is not a directory";
      return false;
    }

    std::vector<std::string> files;

    unsigned int input_blocks_ = 0;
    for(auto& entry : boost::make_iterator_range(fs::directory_iterator(p), {})) {
      files.push_back(entry.path().string());
    }

    for (size_t i = 0; i < files.size(); ++i) {
        LOG_INFO << "Reading " << files.at(i);
        std::ifstream file(files.at(i), std::ios::binary);
        file.unsetf(std::ios::skipws);
        std::streampos file_size;
        file.seekg(0, std::ios::end);
        file_size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<byte> raw;
        raw.reserve(file_size);
        raw.insert(raw.begin(), std::istream_iterator<byte>(file)
                   , std::istream_iterator<byte>());
        size_t offset = 0;
        assert(file_size > 0);
        bool isBlock = isBlockData(raw);
        bool isTransaction = isTxData(raw);
        if (isBlock) LOG_INFO << files.at(i) << " has blocks.";
        if (isTransaction) LOG_INFO << files.at(i) << " has transactions.";
        if (!isBlock && !isTransaction) LOG_WARNING << files.at(i) << " contains unknown data.";
        while (offset < static_cast<size_t>(file_size)) {
          if (isBlock) {
            FinalBlock one_block(raw, posteri, offset);
            posteri = one_block.getChainState();
          } else if (isTransaction) {
            Tier2Transaction tx(raw, offset, keys, true);
            if (!tx.isValid(priori, keys, summary)) {
              LOG_WARNING << "A transaction is invalid. TX details: ";
              LOG_WARNING << tx.getJSON();
            }
          }
        }
      }

    if (!CompareChainStateMaps(priori.stateMap_, posteri.stateMap_)) {
      LOG_WARNING << "End states do not match!";
      LOG_WARNING << "Prior state: "+WriteChainStateMap(priori.stateMap_);
      LOG_WARNING << "Post state: "+WriteChainStateMap(posteri.stateMap_);
    } else {
      LOG_INFO << "End states match.";
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
