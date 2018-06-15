
/*
 * announcer.cpp reads Devcash transaction files from a directory
 * and annouonces them to nodes provided by the host-list arguments
 *
 *  Created on: May 29, 2018
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
#define DEBUG_TRANSACTION_INDEX \
  (processed + 11000000)
typedef std::chrono::milliseconds millisecs;

std::unique_ptr<io::TransactionServer> create_transaction_server(const devcash_options& options,
                                                                 zmq::context_t& context) {
  std::unique_ptr<io::TransactionServer> server(new io::TransactionServer(context,
                                                                          options.bind_endpoint));
  return server;
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

    LOG_DEBUG << "Loading transactions from " << options->scan_dir;
    MTR_SCOPE_FUNC();
    std::vector<std::vector<byte>> transactions;

    ChainState priori;

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

    ThreadPool::ParallelFor(0, (int)files.size(), [&] (int i) {
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
        std::vector<byte> batch;
        assert(file_size > 0);
        while (offset < static_cast<size_t>(file_size)) {
          //constructor increments offset by reference
          FinalBlock one_block(raw, priori, offset);
          Summary sum(one_block.getSummary());
          Validation val(one_block.getValidation());
          std::pair<Address, Signature> pair(val.getFirstValidation());
          int index = keys.getNodeIndex(pair.first);
          Tier1Transaction tx(sum, pair.second, (uint64_t) index, keys);
          std::vector<byte> tx_canon(tx.getCanonical());
          batch.insert(batch.end(), tx_canon.begin(), tx_canon.end());
          input_blocks_++;
        }

        transactions.push_back(batch);
    }, 3);

    LOG_INFO << "Loaded " << std::to_string(input_blocks_) << " transactions in " << transactions.size() << " batches.";

    std::unique_ptr<io::TransactionServer> server = create_transaction_server(*options, context);
    server->startServer();
    auto ms = kMAIN_WAIT_INTERVAL;
    unsigned int processed = 0;
    while (true) {
      LOG_DEBUG << "Sleeping for " << ms
                << ": processed/batches ("
                << std::to_string(processed) << "/"
                << transactions.size() << ")";
      std::this_thread::sleep_for(millisecs(ms));

      /* Should we announce a transaction? */
      if (processed < transactions.size()) {
        for (auto i : options->host_vector) {
          auto announce_msg = std::make_unique<DevcashMessage>(i
                                                         , TRANSACTION_ANNOUNCEMENT
                                                         , transactions.at(processed)
                                                         , DEBUG_TRANSACTION_INDEX);
          server->queueMessage(std::move(announce_msg));
        }
        processed++;
      } else {
        LOG_INFO << "Finished announcing transactions.";
        break;
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
