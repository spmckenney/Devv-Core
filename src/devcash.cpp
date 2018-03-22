
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

bool AppInit(int argc, char* argv[]) {
  CASH_TRY {

    std::string announce("Check DevCash logs at ");
    announce += LOGFILE;
    announce += "\n";
    std::cout << announce;
    LOG_INFO << "DevCash initializing...\n";

    std::string mode(argv[1]);
    int nodeIndex = std::stoi(argv[2]);

    /*CASH_TRY {
      dCashArgs.ReadConfigFile(argv[]);
    } CASH_CATCH (const std::exception& e) {
      LOG_FATAL << "Error reading configuration file: " << e.what();
      return false;
    }*/

    DevcashNode thisNode(mode, nodeIndex);

    std::string inRaw = ReadFile(argv[3]);
    std::string out("");
    if (thisNode.appContext.appMode == scan) {
      LOG_INFO << "Scanner ignores node index.\n";
      out = thisNode.RunScanner(inRaw);
    } else {
      if (!thisNode.Init()) {
        LOG_FATAL << "Basic setup failed";
        return false;
      }
      LOG_INFO << "Basic Setup complete\n";
      if (!thisNode.SanityChecks()) {
        LOG_FATAL << "Sanity checks failed";
        return false;
      }
      LOG_INFO << "Sanity checks passed\n";
      out = thisNode.RunNode(inRaw);
    }

    std::string outFileStr(argv[4]);
    std::ofstream outFile(outFileStr);
    if (outFile.is_open()) {
      outFile << out;
      outFile.close();
    } else {
        LOG_FATAL << "Failed to open output file '"+outFileStr+"'.\n";
        return(false);
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

int main(int argc, char* argv[])
{
  if (argc != 5) {
    LOG_INFO << "Usage: DevCash T1|T2|scan nodeindex inputfile outputfile";
    return(EXIT_FAILURE);
  }

  return(AppInit(argc, argv) ? EXIT_SUCCESS : EXIT_FAILURE);
}
