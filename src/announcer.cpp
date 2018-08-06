
/*
 * announcer.cpp reads Devcash transaction files from a directory
 * and annouonces them to nodes provided by the host-list arguments
 *
 *  Created on: May 29, 2018
 *  Author: Nick Williams
 */

#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "common/logger.h"
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

/**
 * Holds command-line options
 */
struct announcer_options {
  std::string bind_endpoint;
  eAppMode mode;
  unsigned int node_index;
  unsigned int shard_index;
  std::string working_dir;
  std::string inn_keys;
  std::string node_keys;
  std::string key_pass;
  std::string stop_file;
  eDebugMode debug_mode;
  unsigned int batch_size;
  unsigned int start_delay;
  unsigned int sleep_ms;
  bool distinct_ops;
};

/**
 * Parse command-line options
 * @param argc
 * @param argv
 * @return
 */
std::unique_ptr<struct announcer_options> ParseAnnouncerOptions(int argc, char** argv);


int main(int argc, char* argv[]) {
  init_log();

  try {
    auto options = ParseAnnouncerOptions(argc, argv);

    if (!options) {
      LOG_ERROR << "ParseAnnouncerOptions error";
      exit(-1);
    }

    zmq::context_t context(1);

    DevcashContext this_context(options->node_index, options->shard_index, options->mode, options->inn_keys,
                                options->node_keys, options->key_pass);
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
      unsigned int batch_blocks = 0;
      unsigned char last_op = 99;
      bool first_file_tx = true;

      InputBuffer buffer(raw);
      while (buffer.getOffset() < static_cast<size_t>(file_size)) {
        //constructor increments offset by reference
        if (is_block && options->mode == eAppMode::T1) {
          FinalBlock one_block(FinalBlock::Create(buffer, priori));
          const Summary &sum = one_block.getSummary();
          Validation val(one_block.getValidation());
          std::pair<Address, Signature> pair(val.getFirstValidation());
          Tier1Transaction tx(sum, pair.second, pair.first, keys);
          std::vector<byte> tx_canon(tx.getCanonical());
          batch.insert(batch.end(), tx_canon.begin(), tx_canon.end());
          input_blocks_++;
        } else if (is_transaction && options->mode != eAppMode::T1) {
          Tier2Transaction tx(Tier2Transaction::Create(buffer, keys, true));
          std::vector<byte> tx_canon(tx.getCanonical());
          if (options->distinct_ops) {
             if ((!first_file_tx)
                  && (tx.getOperation() != last_op)) {
                LOG_DEBUG << "(!first_file_tx) && (tx.getOperation() != last_op): batch size: " << batch.size();
                ParallelPush(tx_lock, transactions, batch);
                batch.clear();
                batch_blocks = 0;
             } else {
               LOG_DEBUG << "Setting first_file_tx to false";
               first_file_tx = false;
             }
              last_op = tx.getOperation();
          }
          if (options->batch_size <= batch_blocks) {
            ParallelPush(tx_lock, transactions, batch);
            batch.clear();
            batch_blocks = 0;
          }
          batch.insert(batch.end(), tx_canon.begin(), tx_canon.end());
          input_blocks_++;
          batch_blocks++;
        } else {
          LOG_WARNING << "Unsupported configuration: is_transaction: " << is_transaction
                << " and mode: " << options->mode;
          throw std::runtime_error("Unsupported configuration: is_transaction");
        }
      }

      ParallelPush(tx_lock, transactions, batch);
    }, 3);

    LOG_INFO << "Loaded " << std::to_string(input_blocks_) << " transactions in " << transactions.size() << " batches.";

    auto server = io::CreateTransactionServer(options->bind_endpoint, context);
    server->startServer();
    auto ms = options->sleep_ms;
    unsigned int processed = 0;

    if (options->start_delay > 0) sleep(options->start_delay);
    while (true) {
      if (ms > 0) {
        LOG_DEBUG << "Sleeping for " << ms << ": processed/batches (" << std::to_string(processed) << "/"
                  << transactions.size() << ")";
        sleep(ms);
      }


      /* Should we announce a transaction? */
      if (processed < transactions.size()) {
        auto announce_msg = std::make_unique<DevcashMessage>(this_context.get_uri(), TRANSACTION_ANNOUNCEMENT, transactions.at(processed),
                                                               DEBUG_TRANSACTION_INDEX);
        server->queueMessage(std::move(announce_msg));
        LOG_DEBUG << "Sent transaction batch #" << processed << ", size(" << transactions.at(processed).size() << ")";
        ++processed;
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

std::unique_ptr<struct announcer_options> ParseAnnouncerOptions(int argc, char** argv) {

  namespace po = boost::program_options;

  std::unique_ptr<announcer_options> options(new announcer_options());
  std::vector<std::string> config_filenames;

  try {
    po::options_description general("General Options\n\
" + std::string(argv[0]) + " [OPTIONS] \n\
\n\
Reads Devcash transaction files from a directory and \n\
annouonces them to nodes provided by the host-list arguments.\n\
\nAllowed options");
    general.add_options()
        ("help,h", "produce help message")
        ("version,v", "print version string")
        ("config", po::value(&config_filenames), "Config file where options may be specified (can be specified more than once)")
        ;

    po::options_description behavior("Identity and Behavior Options");
    behavior.add_options()
        ("debug-mode", po::value<std::string>(), "Debug mode (on|toy|perf) for testing")
        ("mode", po::value<std::string>(), "Devcash mode (T1|T2|scan)")
        ("node-index", po::value<unsigned int>(), "Index of this node")
        ("shard-index", po::value<unsigned int>(), "Index of this shard")
        ("num-consensus-threads", po::value<unsigned int>(), "Number of consensus threads")
        ("num-validator-threads", po::value<unsigned int>(), "Number of validation threads")
        ("bind-endpoint", po::value<std::string>(), "Endpoint for server (i.e. tcp://*:5556)")
        ("working-dir", po::value<std::string>(), "Directory where inputs are read and outputs are written")
        ("output", po::value<std::string>(), "Output path in binary JSON or CBOR")
        ("trace-output", po::value<std::string>(), "Output path to JSON trace file (Chrome)")
        ("inn-keys", po::value<std::string>(), "Path to INN key file")
        ("node-keys", po::value<std::string>(), "Path to Node key file")
        ("key-pass", po::value<std::string>(), "Password for private keys")
        ("generate-tx", po::value<unsigned int>(), "Generate at least this many Transactions")
        ("tx-batch-size", po::value<unsigned int>(), "Target size of transaction batches")
        ("stop-file", po::value<std::string>(), "A file in working-dir indicating that this node should stop.")
        ("start-delay", po::value<unsigned int>(), "Sleep time before starting (millis)")
        ("sleep-ms", po::value<unsigned int>(), "Sleep time between batches (millis)")
        ("separate-ops", po::value<bool>(), "Separate transactions with different operations into distinct batches?")
        ;

    po::options_description all_options;
    all_options.add(general);
    all_options.add(behavior);

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

    if (vm.count("bind-endpoint")) {
      options->bind_endpoint = vm["bind-endpoint"].as<std::string>();
      LOG_INFO << "Bind URI: " << options->bind_endpoint;
    } else {
      LOG_INFO << "Bind URI was not set";
    }

    if (vm.count("working-dir")) {
      options->working_dir = vm["working-dir"].as<std::string>();
      LOG_INFO << "Working dir: " << options->working_dir;
    } else {
      LOG_INFO << "Working dir was not set.";
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

    if (vm.count("tx-batch-size")) {
      options->batch_size = vm["tx-batch-size"].as<unsigned int>();
      LOG_INFO << "Batch size: " << options->start_delay;
    } else {
      LOG_INFO << "Batch size was not set (default to no restrictions).";
      options->batch_size = 0;
    }

    if (vm.count("start-delay")) {
      options->start_delay = vm["start-delay"].as<unsigned int>();
      LOG_INFO << "Start delay: " << options->start_delay;
    } else {
      LOG_INFO << "Start delay was not set (default to no delay).";
      options->start_delay = 0;
    }

    if (vm.count("sleep-ms")) {
      options->sleep_ms = vm["sleep-ms"].as<unsigned int>();
      LOG_INFO << "Sleep millis: " << options->sleep_ms;
    } else {
      LOG_INFO << "Sleep millis was not set (default to no waiting).";
      options->sleep_ms = 0;
    }

    if (vm.count("separate-ops")) {
      options->distinct_ops = vm["separate-ops"].as<bool>();
      LOG_INFO << "Separate admin ops: " << options->distinct_ops;
    } else {
      LOG_INFO << "Separate ops was not set (default to no separation).";
      options->distinct_ops = false;
    }

  }
  catch(std::exception& e) {
    LOG_ERROR << "error: " << e.what();
    return nullptr;
  }

  return options;
}
