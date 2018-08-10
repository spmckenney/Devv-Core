
/*
 * repeater.cpp listens for FinalBlock messages and saves them to a file
 * @TODO(nick@cloudsolar.co): it also provides specific blocks by request.
 *
 *  Created on: June 25, 2018
 *  Author: Nick Williams
 */

#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <pqxx/pqxx>

#include "common/logger.h"
#include "common/devcash_context.h"
#include "io/message_service.h"
#include "modules/BlockchainModule.h"

using namespace Devcash;

namespace fs = boost::filesystem;

#define DEBUG_TRANSACTION_INDEX (processed + 11000000)
typedef std::chrono::milliseconds millisecs;

/**
 * Holds command-line options
 */
struct repeater_options {
  std::vector<std::string> host_vector{};
  eAppMode mode = eAppMode::T1;
  unsigned int node_index = 0;
  unsigned int shard_index = 0;
  std::string working_dir;
  std::string inn_keys;
  std::string node_keys;
  std::string key_pass;
  std::string stop_file;
  eDebugMode debug_mode = eDebugMode::off;

  std::string db_user;
  std::string db_pass;
  std::string db_host;
  std::string db_name;
  unsigned int db_port = 5432;
};

static const std::string kNIL_UUID = "00000000-0000-0000-0000-000000000000";
static const std::string kNIL_UUID_PSQL = "'00000000-0000-0000-0000-000000000000'::uuid";
static const std::string kTX_INSERT = "tx_insert";
static const std::string kTX_INSERT_STATEMENT = "INSERT INTO tx (tx_id, shard, block_height, wallet_id, coin_id, amount) VALUES ($1, $2, $3, $4, $5, $6)";
static const std::string kRX_INSERT = "rx_insert";
static const std::string kRX_INSERT_STATEMENT = "INSERT INTO rx (shard, block_height, wallet_id, coin_id, amount, tx_id) VALUES ($1, $2, $3, $4, $5, $6)";
static const std::string kBALANCE_SELECT = "balance_select";
static const std::string kBALANCE_SELECT_STATEMENT = "select balance from walletCoins where wallet_id = $1 and coin_id = $2";
static const std::string kWALLET_INSERT = "wallet_insert";
static const std::string kWALLET_INSERT_STATEMENT = "INSERT INTO wallet (wallet_id, account_id, wallet_name) VALUES ($1, '"+kNIL_UUID+"':uuid, 'None')";
static const std::string kBALANCE_INSERT = "balance_insert";
static const std::string kBALANCE_INSERT_STATEMENT = "INSERT INTO walletCoins (wallet_coin_id, wallet_id, coin_id, account_id, balance) (select devv_uuid(), $1, $2, $3, $4)";
static const std::string kBALANCE_UPDATE = "balance_update";
static const std::string kBALANCE_UPDATE_STATEMENT = "UPDATE walletCoins set balance = $1 where wallet_id = $2 and coin_id = $3";


/**
 * Parse command-line options
 * @param argc
 * @param argv
 * @return
 */
std::unique_ptr<struct repeater_options> ParseRepeaterOptions(int argc, char** argv);

int main(int argc, char* argv[]) {
  init_log();

  try {
    auto options = ParseRepeaterOptions(argc, argv);

    if (!options) {
      exit(-1);
    }

    zmq::context_t zmq_context(1);

    DevcashContext this_context(options->node_index, options->shard_index, options->mode, options->inn_keys,
                                options->node_keys, options->key_pass);
    KeyRing keys(this_context);

    LOG_DEBUG << "Write transactions to " << options->working_dir;
    MTR_SCOPE_FUNC();

    fs::path p(options->working_dir);

    if (!is_directory(p)) {
      LOG_ERROR << "Error opening dir: " << options->working_dir << " is not a directory";
      return false;
    }

    std::string shard_name = "Shard-"+std::to_string(options->shard_index);
    bool db_connected = false;
    if (!options->db_user.empty() && !options->db_host.empty()
       && !options->db_name.empty() && !options->db_pass.empty()) {
      pqxx::connection db_link(
        "username="+options->db_user+
        " host="+options->db_host+
        " password="+options->db_pass+
        " dbname="+options->db_name+
        " port="+std::to_string(options->db_port));
      if (db_link.is_open()) {
        LOG_INFO << "Successfully connected to database.";
        db_connected = true;
        db_link.prepare(kTX_INSERT, kTX_INSERT_STATEMENT);
        db_link.prepare(kRX_INSERT, kRX_INSERT_STATEMENT);
        db_link.prepare(kWALLET_INSERT, kWALLET_INSERT_STATEMENT);
        db_link.prepare(kBALANCE_SELECT, kBALANCE_SELECT_STATEMENT);
        db_link.prepare(kBALANCE_INSERT, kBALANCE_INSERT_STATEMENT);
        db_link.prepare(kBALANCE_UPDATE, kBALANCE_UPDATE_STATEMENT);
	  } else {
        LOG_INFO << "Database connection failed.";
	  }
    }

    //@todo(nick@cloudsolar.co): read pre-existing chain
    unsigned int chain_height = 0;

    auto peer_listener = io::CreateTransactionClient(options->host_vector, zmq_context);
    peer_listener->attachCallback([&](DevcashMessageUniquePtr p) {
      if (p->message_type == eMessageType::FINAL_BLOCK) {
        //update database
        if (db_connected) {
          std::vector<TransactionPtr> txs = p->CopyTransactions();
          for (TransactionPtr& one_tx : txs) {
			pqxx::work stmt(db_link);
			std::vector<TransferPtr> xfers = txs->getTransfers();
			Siganture sig = txs->getSignature();
			Address sender;
			uint64_t coin_id = 0;
			int64_t send_amount = 0;
            for (TransferPtr& one_xfer : xfers) {
              if (one_xfer->getAmount() < 0) {
                sender = one_xfer.getAddress();
                coin_id = one_xfer.getCoin();
                send_amount = one_xfer.getAmount();
                break;
			  }
			} //end sender search loop

			//update sender
			pqxx::result balance_result = stmt.prepared(kBALANCE_SELECT)(sender.getCanonical())(coin_id).exec();
			if (balance_result.empty()) {
              stmt.prepared(kBALANCE_INSERT)(sender.getCanonical())(coin_id)(kNIL_UUID_PSQL)(send_amount).exec();
			} else {
              int64_t new_balance = balance_result[0][0].as<int64_t>()-send_amount;
              stmt.prepared(kBALANCE_UPDATE)(new_balance)(sender.getCanonical())(coin_id).exec();
			}
			stmt.prepared(kTX_INSERT)(sig.getCanonical())(shard_name)(chain_height)(sender.getCanonical())(coin_id)(send_amount).exec();
            for (TransferPtr& one_xfer : xfers) {
			  int64_t amount = one_xfer->getAmount();
              if (amount < 0) continue;
              //update receiver
              Address receiver = one_xfer->getAddress();
              pqxx::work rx_stmt(db_link);
              pqxx::result rx_balance = stmt.prepared(kBALANCE_SELECT)(receiver.getCanonical())(coin_id).exec();
              if (rx_balance.empty()) {
                stmt.prepared(kBALANCE_INSERT)(receiver.getCanonical())(coin_id)(kNIL_UUID_PSQL)(amount).exec();
              } else {
                int64_t new_balance = balance_result[0][0].as<int64_t>()+amount;
                stmt.prepared(kBALANCE_UPDATE)(new_balance)(reciever.getCanonical())(coin_id).exec();
			  }
			  stmt.prepared(kRX_INSERT)(shard_name)(chain_height)(receiver.getCanonical())(coin_id)(send_amount)(sig.getCanonical()).exec();
			} //end transfer loop
			stmt.commit();
          } //end transaction loop
        } //endif db connected?

        //write final chain to file
        std::string shard_dir(options->working_dir+"/"+this_context.get_shard_uri());
        fs::path dir_path(shard_dir);
        if (is_directory(dir_path)) {
          std::string block_height(std::to_string(chain_height));
          std::string out_file(shard_dir + "/" + block_height + ".dat");
          std::ofstream block_file(out_file
            , std::ios::out | std::ios::binary);
          if (block_file.is_open()) {
            block_file.write((const char*) &p->data[0], p->data.size());
            block_file.close();
            LOG_DEBUG << "Wrote to " << out_file << "'.";
          } else {
            LOG_ERROR << "Failed to open output file '" << out_file << "'.";
          }
        } else {
          LOG_ERROR << "Error opening dir: " << shard_dir << " is not a directory";
        }
        chain_height++;
      }
    });
    peer_listener->listenTo(this_context.get_shard_uri());
    peer_listener->startClient();
    LOG_INFO << "Repeater is listening to shard: "+this_context.get_shard_uri();

    auto ms = kMAIN_WAIT_INTERVAL;
    while (true) {
      LOG_DEBUG << "Repeater sleeping for " << ms;
      std::this_thread::sleep_for(millisecs(ms));
      /* Should we shutdown? */
      if (fs::exists(options->stop_file)) {
        LOG_INFO << "Shutdown file exists. Stopping repeater...";
        break;
      }
    }
    if (db_connected) db_link.disconnect();
    peer_listener->stopClient();
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


std::unique_ptr<struct repeater_options> ParseRepeaterOptions(int argc, char** argv) {

  namespace po = boost::program_options;

  std::unique_ptr<repeater_options> options(new repeater_options());
  std::vector<std::string> config_filenames;

  try {
    po::options_description general("General Options\n\
" + std::string(argv[0]) + " [OPTIONS] \n\
Listens for FinalBlock messages and saves them to a file\n\
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
        ("host-list,host", po::value<std::vector<std::string>>(),
         "Client URI (i.e. tcp://192.168.10.1:5005). Option can be repeated to connect to multiple nodes.")
        ("working-dir", po::value<std::string>(), "Directory where inputs are read and outputs are written")
        ("output", po::value<std::string>(), "Output path in binary JSON or CBOR")
        ("trace-output", po::value<std::string>(), "Output path to JSON trace file (Chrome)")
        ("inn-keys", po::value<std::string>(), "Path to INN key file")
        ("node-keys", po::value<std::string>(), "Path to Node key file")
        ("key-pass", po::value<std::string>(), "Password for private keys")
        ("generate-tx", po::value<unsigned int>(), "Generate at least this many Transactions")
        ("tx-batch-size", po::value<unsigned int>(), "Target size of transaction batches")
        ("tx-limit", po::value<unsigned int>(), "Number of transaction to process before shutting down.")
        ("stop-file", po::value<std::string>(), "A file in working-dir indicating that this node should stop.")
        ("update-host", po::value<std::string>(), "Host of database to update.")
        ("update-db", po::value<std::string>(), "Database name to update.")
        ("update-user", po::value<std::string>(), "Database username to use for updates.")
        ("update-pass", po::value<std::string>(), "Database password to use for updates.")
        ("update-port", po::value<unsigned int>(), "Database port to use for updates.")
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

    if (vm.count("host-list")) {
      options->host_vector = vm["host-list"].as<std::vector<std::string>>();
      LOG_INFO << "Node URIs:";
      for (auto i : options->host_vector) {
        LOG_INFO << "  " << i;
      }
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

    if (vm.count("update-host")) {
      options->db_host = vm["update-host"].as<std::string>();
      LOG_INFO << "Database host: " << options->db_host;
    } else {
      LOG_INFO << "Database host was not set.";
    }

    if (vm.count("update-db")) {
      options->db_name = vm["update-db"].as<std::string>();
      LOG_INFO << "Database name: " << options->db_name;
    } else {
      LOG_INFO << "Database name was not set.";
    }

    if (vm.count("update-user")) {
      options->db_user = vm["update-user"].as<std::string>();
      LOG_INFO << "Database user: " << options->db_host;
    } else {
      LOG_INFO << "Database user was not set.";
    }

    if (vm.count("update-pass")) {
      options->db_pass = vm["update-pass"].as<std::string>();
      LOG_INFO << "Database pass: " << options->db_pass;
    } else {
      LOG_INFO << "Database pass was not set.";
    }

    if (vm.count("update-port")) {
      options->db_port = vm["update-port"].as<unsigned int>();
      LOG_INFO << "Database pass: " << options->db_port;
    } else {
      LOG_INFO << "Database port was not set, default to 5432.";
      options->db_port = 5432;
    }

  }
  catch(std::exception& e) {
    LOG_ERROR << "error: " << e.what();
    return nullptr;
  }

  return options;
}
