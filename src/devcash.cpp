
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
#include "common/json.hpp"
#include "common/logger.h"
#include "common/util.h"

#include "common/argument_parser.h"

#include "io/message_service.h"

using namespace Devcash;
using json = nlohmann::json;

//ArgsManager dCashArgs; /** stores data parsed from config file */

//toggle exceptions on/off
#if (defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)) && not defined(DEVCASH_NOEXCEPTION)
    #define CASH_THROW(exception) throw exception
    #define CASH_TRY try
    #define CASH_CATCH(exception) catch(exception)
#else
    #define CASH_THROW(exception) std::abort()
    #define CASH_TRY if(true)
    #define CASH_CATCH(exception) if(false)
#endif

typedef unsigned char byte;
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
  CASH_TRY {
    std::unique_ptr<devcash_options> options = parse_options(argc, argv);

    if (!options) {
      exit(-1);
    }

    zmq::context_t context(1);

    std::unique_ptr<io::TransactionServer> server = create_transaction_server(*options, context);
    std::unique_ptr<io::TransactionClient> client = create_transaction_client(*options, context);

    DevcashContext this_context(options->node_index,
                                static_cast<eAppMode>(options->mode));
    KeyRing keys(this_context);
    ProposedBlock genesis;
    ProposedBlock on_deck;

    DevcashController controller(*server,
                                 *client,
      options->num_validator_threads, options->num_consensus_threads,
      keys, this_context,genesis,on_deck);

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
      keys.initKeys();
      out = this_node.RunNetworkTest();
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
