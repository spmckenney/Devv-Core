
/*
 * devcash.cpp the main class.  Checks args and hands of to init.
 *
 *  Created on: Dec 8, 2017
 *  Author: Nick Williams
 */

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <functional>
#include <thread>

#include "common/devcash_context.h"
#include "concurrency/DevcashController.h"
#include "devcashnode.h"
#include "common/logger.h"
#include "common/util.h"

#include "common/argument_parser.h"

#include "io/message_service.h"

using namespace Devcash;

#define UNUSED(x) ((void)x)

std::unique_ptr<io::TransactionClient> create_transaction_client(const devcash_options& options,
                                                                 zmq::context_t& context) {
  std::unique_ptr<io::TransactionClient> client(new io::TransactionClient(context));
  for (auto i : options.host_vector) {
    client->AddConnection(i);
  }
  return client;
}

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

    DevcashContext this_context(options->node_index,
                                static_cast<eAppMode>(options->mode));
    KeyRing keys(this_context);
    ChainState prior;

    std::unique_ptr<io::TransactionServer> server = create_transaction_server(*options, context);
    std::unique_ptr<io::TransactionClient> peer_client = create_transaction_client(*options, context);

    // Create loopback client to subscribe to simulator transactions
    std::unique_ptr<io::TransactionClient> loopback_client(new io::TransactionClient(context));
    auto be = options->bind_endpoint;
    std::string this_uri = "";
    try {
      this_uri = "tcp://localhost" + be.substr(be.rfind(":"));
    } catch (std::range_error& e) {
      LOG_ERROR << "Extracting bind number failed: " << be;
    }
    loopback_client->AddConnection(this_uri);

    DevcashController controller(*server, *peer_client, *loopback_client,
      options->num_validator_threads, options->num_consensus_threads,
      options->generate_count, options->tx_batch_size,
      keys, this_context, prior);

    DevcashNode this_node(controller, this_context);

    std::string in_raw = ReadFile(options->scan_file);

    std::string out("");
    if (options->mode == eAppMode::scan) {
      LOG_INFO << "Scanner ignores node index.";
      out = this_node.RunScanner(in_raw);
    } else {
      if (!this_node.Init()) {
        LOG_FATAL << "Basic setup failed";
        return false;
      }
      LOG_INFO << "Basic Setup complete";
      if (!this_node.SanityChecks()) {
        LOG_FATAL << "Sanity checks failed";
        return false;
      }
      LOG_INFO << "Sanity checks passed";
      if (options->debug_mode == eDebugMode::toy) {
        this_node.RunNetworkTest(options->node_index);
      }
      out = this_node.RunNode(in_raw);
    }

    //We do need to output the resulting blockchain for analysis
    //It should be deterministic based on the input and parameters,
    //so we won't need it from every run, just interesting ones.
    //Feel free to do this differently!
    std::string outFileStr(argv[4]);
    std::ofstream outFile(options->write_file);
    if (outFile.is_open()) {
      outFile << out;
      outFile.close();
    } else {
      LOG_FATAL << "Failed to open output file '" << outFileStr << "'.";
      return(false);
    }

    LOG_INFO << "DevCash Shutting Down";
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
