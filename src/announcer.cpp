
/*
 * announcer.cpp reads Devcash transaction files from a directory
 * and annouonces them to nodes provided by the host-list arguments
 *
 *  Created on: May 29, 2018
 *  Author: Nick Williams
 */

#include <boost/filesystem.hpp>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

#include "common/logger.h"
#include "common/argument_parser.h"
#include "common/devcash_context.h"
#include "io/message_service.h"
#include "modules/BlockchainModule.h"
#include "primitives/block_tools.h"

using namespace Devcash;

namespace fs = boost::filesystem;

#define DEBUG_TRANSACTION_INDEX (processed + 11000000)

void ParallelPush(std::mutex& m, std::vector<std::vector<byte>>& array
    , const std::vector<byte>& elt) {
  std::lock_guard<std::mutex> guard(m);
  array.push_back(elt);
}

int main(int argc, char* argv[]) {
  init_log();

  try {
    std::unique_ptr<devcash_options> options = parse_options(argc, argv);

    if (!options) {
      LOG_ERROR << "parse_options error";
      exit(-1);
    }

    zmq::context_t context(1);

    DevcashContext this_context(options->node_index, options->shard_index, options->mode, options->inn_keys,
                                options->node_keys, options->wallet_keys);
    KeyRing keys(this_context);

    LOG_DEBUG << "Loading transactions from " << options->working_dir;
    MTR_SCOPE_FUNC();
    std::vector<std::vector<byte>> transactions;
    std::mutex tx_lock;

    ChainState priori;

    fs::path p(options->working_dir);

    if (!is_directory(p)) {
      LOG_ERROR << "Error opening dir: " << options->working_dir << " is not a directory";
      return false;
    }

    std::vector<std::string> files;

    unsigned int input_blocks_ = 0;
    for (auto &entry : boost::make_iterator_range(fs::directory_iterator(p), {})) {
      files.push_back(entry.path().string());
    }

    ThreadPool::ParallelFor(0, (int) files.size(), [&](int i) {
      LOG_INFO << "Reading " << files.at(i);
      std::ifstream file(files.at(i), std::ios::binary);
      file.unsetf(std::ios::skipws);
      std::streampos file_size;
      file.seekg(0, std::ios::end);
      file_size = file.tellg();
      file.seekg(0, std::ios::beg);

      std::vector<byte> raw;
      raw.reserve(file_size);
      raw.insert(raw.begin(), std::istream_iterator<byte>(file), std::istream_iterator<byte>());
      std::vector<byte> batch;
      assert(file_size > 0);
      bool is_block = IsBlockData(raw);
      bool is_transaction = IsTxData(raw);
      if (is_block) { LOG_INFO << files.at(i) << " has blocks."; }
      if (is_transaction) { LOG_INFO << files.at(i) << " has transactions."; }
      if (!is_block && !is_transaction) { LOG_WARNING << files.at(i) << " contains unknown data."; }

      InputBuffer buffer(raw);
      while (buffer.getOffset() < static_cast<size_t>(file_size)) {
        //constructor increments offset by reference
        if (is_block && options->mode == eAppMode::T1) {
          FinalBlock one_block(FinalBlock::Create(buffer, priori));
          const Summary &sum = one_block.getSummary();
          Validation val(one_block.getValidation());
          std::pair<Address, Signature> pair(val.getFirstValidation());
          int index = keys.getNodeIndex(pair.first);
          Tier1Transaction tx(sum, pair.second, (uint64_t) index, keys);
          std::vector<byte> tx_canon(tx.getCanonical());
          batch.insert(batch.end(), tx_canon.begin(), tx_canon.end());
          input_blocks_++;
        } else if (is_transaction && options->mode == eAppMode::T2) {
          Tier2Transaction tx(Tier2Transaction::Create(buffer, keys, true));
          std::vector<byte> tx_canon(tx.getCanonical());
          batch.insert(batch.end(), tx_canon.begin(), tx_canon.end());
          input_blocks_++;
        } else {
          LOG_WARNING << "Unsupported configuration: is_transaction: " << is_transaction
                << " and mode: " << options->mode;
        }
      }

      ParallelPush(tx_lock, transactions, batch);
    }, 3);

    LOG_INFO << "Loaded " << std::to_string(input_blocks_) << " transactions in " << transactions.size() << " batches.";

    std::unique_ptr<io::TransactionServer> server = io::CreateTransactionServer(options->bind_endpoint, context);
    server->startServer();
    auto ms = kMAIN_WAIT_INTERVAL;
    unsigned int processed = 0;

    //LOG_NOTICE << "Please press a key to ignore";
    std::cin.ignore(); //why read something if you need to ignore it? :)
    while (true) {
      LOG_DEBUG << "Sleeping for " << ms << ": processed/batches (" << std::to_string(processed) << "/"
                << transactions.size() << ")";

      /* Should we announce a transaction? */
      if (processed < transactions.size()) {
        auto announce_msg = std::make_unique<DevcashMessage>(this_context.get_uri(), TRANSACTION_ANNOUNCEMENT, transactions.at(processed),
                                                               DEBUG_TRANSACTION_INDEX);
        server->queueMessage(std::move(announce_msg));
        ++processed;
        LOG_DEBUG << "Sent transaction batch #" << processed;
        sleep(1);
      } else {
        LOG_INFO << "Finished announcing transactions.";
        break;
      }
      LOG_INFO << "Finished 0";
    }
    LOG_INFO << "Finished 1";
    server->stopServer();
    LOG_WARNING << "All done.";
    return (true);
  }
  catch (const std::exception& e) {
    std::exception_ptr p = std::current_exception();
    std::string err("");
    err += (p ? p.__cxa_exception_type()->name() : "null");
    LOG_FATAL << "Error: " + err << std::endl;
    std::cerr << err << std::endl;
    return (false);
  }
}
