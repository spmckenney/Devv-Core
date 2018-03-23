
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

#if 0
bool AppInit(int, char* argv[]) {
  CASH_TRY {

    std::string announce("Check DevCash logs at ");
    announce += LOGFILE;
    announce += "\n";
    std::cout << announce;
    LOG_INFO << "DevCash initializing...\n";

    /*CASH_TRY {
      dCashArgs.ReadConfigFile(argv[]);
    } CASH_CATCH (const std::exception& e) {
      LOG_FATAL << "Error reading configuration file: " << e.what();
      return false;
    }*/


    /*
    std::string outFileStr(argv[4]);
    std::ofstream outFile(outFileStr);
    if (outFile.is_open()) {
      outFile << out;
      outFile.close();
    } else {
        LOG_FATAL << "Failed to open output file '"+outFileStr+"'.\n";
        return(false);
    }
    */

}
#endif

int main(int argc, char* argv[])
{
  std::unique_ptr<devcash_options> options = parse_options(argc, argv);

  if (!options) {
    exit(-1);
  }
  
  zmq::context_t context(1);

  std::unique_ptr<io::TransactionServer> server = create_transaction_server(*options, context);
  std::unique_ptr<io::TransactionClient> client = create_transaction_client(*options, context);

  DCState chainState;
  ConsensusWorker consensus(chainState, std::move(server), options->num_consensus_threads);
  ValidatorWorker validator(chainState, consensus, options->num_validator_threads);

  CASH_TRY {
    DevcashNode this_node(options->mode
                          , options->node_index
                          , consensus
                          , validator
                          , *client
                          , *server);

    std::string out("");
    if (options->mode == "scan") {
      LOG_INFO << "Scanner ignores node index.\n";
      out = this_node.RunScanner(options->scan_file);
    } else {
      if (!this_node.Init()) {
        LOG_FATAL << "Basic setup failed";
        return false;
      }
      LOG_INFO << "Basic Setup complete\n";
      if (!this_node.SanityChecks()) {
        LOG_FATAL << "Sanity checks failed";
        return false;
      }
      LOG_INFO << "Sanity checks passed\n";
      out = this_node.RunNode();
    }
    LOG_INFO << "DevCash Shutting Down\n";
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
