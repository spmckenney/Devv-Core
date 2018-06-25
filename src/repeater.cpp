
/*
 * repeater.cpp listens for FinalBlock messages and saves them to a file
 * @TODO(nick@cloudsolar.co): it also provides specific blocks by request.
 *
 *  Created on: June 25, 2018
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

using namespace Devcash;

namespace fs = boost::filesystem;

#define DEBUG_TRANSACTION_INDEX (processed + 11000000)
typedef std::chrono::milliseconds millisecs;

std::unique_ptr<io::TransactionClient> create_transaction_client(const devcash_options& options,
                                                                 zmq::context_t& context) {
  std::unique_ptr<io::TransactionClient> client(new io::TransactionClient(context));
  for (auto i : options.host_vector) {
    client->addConnection(i);
  }
  return client;
}

int main(int argc, char* argv[]) {
  init_log();

  try {
    std::unique_ptr<devcash_options> options = parse_options(argc, argv);

    if (!options) {
      exit(-1);
    }

    zmq::context_t context(1);

    DevcashContext this_context(options->node_index, options->shard_index, options->mode, options->inn_keys,
                                options->node_keys, options->wallet_keys);
    KeyRing keys(this_context);

    LOG_DEBUG << "Write transactions to " << options->working_dir;
    MTR_SCOPE_FUNC();

    fs::path p(options->working_dir);

    if (!is_directory(p)) {
      LOG_ERROR << "Error opening dir: " << options->working_dir << " is not a directory";
      return false;
    }

    //@todo(nick@cloudsolar.co): read pre-existing chain
    unsigned int chain_height = 0;

    std::unique_ptr<io::TransactionClient> peer_listener = create_transaction_client(*options, zmq_context);
    peer_listener.attachCallback([&](DevcashMessageUniquePtr p) {
      if (message->message_type == eMessageType::FINAL_BLOCK) {
        //write final chain to file
        std::string shard_dir(options->working_dir+"/"+this_context.get_shard_uri());
        fs::path dir_path(shard_dir);
        if (is_directory(dir_path)) {
          std::string block_height(std::to_string(chain_height));
          std::ofstream block_file(block_height
            , std::ios::out | std::ios::binary);
          if (block_file.is_open()) {
            block_file.write((const char*) &p->data[0], p->data().size());
            block_file.close();
          } else {
            LOG_ERROR << "Failed to open output file '" << shard_dir+"/"+block_height << "'.";
          }
        } else {
          LOG_ERROR << "Error opening dir: " << shard_dir << " is not a directory";
        }
      }
    });
    peer_listener->listenTo(this_context.get_shard_uri());
    peer_listener->startClient();
    LOG_INFO << "Repeater is listening to shard: "+this_context.get_shard_uri();

    while (true) {
      /* Should we shutdown? */
      if (fs::exists(options->stop_file)) {
        LOG_INFO << "Shutdown file exists. Stopping repeater...";
        break;
      }
    }
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
