
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

#include <stdio.h>
#include <stdlib.h>
#include <boost/filesystem.hpp>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

#include "common/argument_parser.h"
#include "common/devcash_context.h"
#include "io/message_service.h"
#include "node/DevcashNode.h"

using namespace Devcash;

namespace fs = boost::filesystem;

#define UNUSED(x) ((void)x)

/** Checks if binary is encoding a block
 * @note this function is pretty heuristic, do not use in critical cases
 * @return true if this data encodes a block
 * @return false otherwise
 */
bool IsBlockData(const std::vector<byte>& raw) {
  //check if big enough
  if (raw.size() < FinalBlock::MinSize()) return false;
  //check version
  if (raw[0] != 0x00) return false;
  size_t offset = 9;
  uint64_t block_time = BinToUint64(raw, offset);
  // check blocktime is from 2018 or newer.
  if (block_time < 1514764800) return false;
  // check blocktime is in past
  if (block_time > GetMillisecondsSinceEpoch()) return false;
  return true;
}

/** Checks if binary is encoding Transactions
 * @note this function is pretty heuristic, do not use in critical cases
 * @return true if this data encodes Transactions
 * @return false otherwise
 */
bool IsTxData(const std::vector<byte>& raw) {
  // check if big enough
  if (raw.size() < Transaction::MinSize()) return false;
  // check transfer count
  uint64_t xfer_count = BinToUint64(raw, 0);
  size_t tx_size = Transaction::MinSize() + (Transfer::Size() * xfer_count);
  if (raw.size() < tx_size) return false;
  // check operation
  if (raw[8] >= 4) return false;
  return true;
}

/** Checks if two chain state maps contain the same state
 * @return true if the maps have the same state
 * @return false otherwise
 */
bool CompareChainStateMaps(const std::map<Address, std::map<uint64_t, uint64_t>>& first,
                           const std::map<Address, std::map<uint64_t, uint64_t>>& second) {
  if (first.size() != second.size()) return false;
  for (auto i = first.begin(), j = second.begin(); i != first.end(); ++i, ++j) {
    if (i->first != j->first) return false;
    if (i->second.size() != j->second.size()) return false;
    for (auto x = i->second.begin(), y = j->second.begin(); x != i->second.end(); ++x, ++y) {
      if (x->first != y->first) return false;
      if (x->second != y->second) return false;
    }
  }
  return true;
}

/** Dumps the map inside a chainstate object into a human-readable JSON format.
 * @return a string containing a description of the chain state.
 */
std::string WriteChainStateMap(const std::map<Address, std::map<uint64_t, uint64_t>>& map) {
  std::string out("{");
  bool first_addr = true;
  for (auto e : map) {
    if (first_addr) {
      first_addr = false;
    } else {
      out += ",";
    }
    Address a = e.first;
    out += "\"" + ToHex(std::vector<byte>(std::begin(a), std::end(a))) + "\":[";
    bool is_first;
    for (auto f : e.second) {
      if (is_first) {
        is_first = false;
      } else {
        out += ",";
      }
      out += std::to_string(f.first) + ":" + std::to_string(f.second);
    }
    out += "]";
  }
  out += "}";
  return out;
}

int main(int argc, char* argv[]) {
  init_log();

  CASH_TRY {
    std::unique_ptr<devcash_options> options = parse_options(argc, argv);

    if (!options) {
      exit(-1);
    }

    zmq::context_t context(1);

    DevcashContext this_context(options->node_index, options->shard_index, options->mode, options->inn_keys,
                                options->node_keys, options->wallet_keys, options->sync_port, options->sync_host);
    KeyRing keys(this_context);

    LOG_DEBUG << "Scanning " << options->scan_dir;
    MTR_SCOPE_FUNC();

    ChainState priori;
    ChainState posteri;
    Summary summary = Summary::Create();

    fs::path p(options->scan_dir);

    if (!is_directory(p)) {
      LOG_ERROR << "Error opening dir: " << options->scan_dir << " is not a directory";
      return false;
    }

    std::vector<std::string> files;

    for (auto& entry : boost::make_iterator_range(fs::directory_iterator(p), {})) {
      files.push_back(entry.path().string());
    }

    for (auto const& file_name : files) {
      LOG_INFO << "Reading " << file_name;
      std::ifstream file(file_name, std::ios::binary);
      file.unsetf(std::ios::skipws);
      std::streampos file_size;
      file.seekg(0, std::ios::end);
      file_size = file.tellg();
      file.seekg(0, std::ios::beg);

      std::vector<byte> raw;
      raw.reserve(file_size);
      raw.insert(raw.begin(), std::istream_iterator<byte>(file), std::istream_iterator<byte>());
      assert(file_size > 0);
      bool is_block = IsBlockData(raw);
      bool is_transaction = IsTxData(raw);
      if (is_block) LOG_INFO << file_name << " has blocks.";
      if (is_transaction) LOG_INFO << file_name << " has transactions.";
      if (!is_block && !is_transaction) LOG_WARNING << file_name << " contains unknown data.";

      InputBuffer buffer(raw);
      while (buffer.getOffset() < static_cast<size_t>(file_size)) {
        if (is_block) {
          size_t span = buffer.getOffset();
          FinalBlock one_block(buffer, posteri, keys, options->mode);
          if (buffer.getOffset() == span) {
            LOG_WARNING << file_name << " has invalid block!";
            break;
          }
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
            LOG_WARNING << one_block.getSummary().getJSON();
            LOG_WARNING << "Transaction details: ";
            for (TransactionPtr& item : txs) {
              LOG_WARNING << item->getJSON();
            }
          }
        } else if (is_transaction) {
          Tier2Transaction tx(buffer, keys, true);
          if (!tx.isValid(priori, keys, summary)) {
            LOG_WARNING << "A transaction is invalid. TX details: ";
            LOG_WARNING << tx.getJSON();
          }
        }
      }
    }

    if (!CompareChainStateMaps(priori.getStateMap(), posteri.getStateMap())) {
      LOG_WARNING << "End states do not match!";
      LOG_WARNING << "Prior state: " + WriteChainStateMap(priori.getStateMap());
      LOG_WARNING << "Post state: " + WriteChainStateMap(posteri.getStateMap());
    } else {
      LOG_INFO << "End states match.";
    }

    if (posteri.getStateMap().empty()) {
      LOG_INFO << "End state is empty.";
    }

    return (true);
  }
  CASH_CATCH(...) {
    std::exception_ptr p = std::current_exception();
    std::string err("");
    err += (p ? p.__cxa_exception_type()->name() : "null");
    LOG_FATAL << "Error: " + err << std::endl;
    std::cerr << err << std::endl;
    return (false);
  }
}
