
/*
 * devcash.cpp
 *
 *  Created on: Dec 8, 2017
 *      Author: Nick Williams
 */
#if defined(HAVE_CONFIG_H)
#include <DevcashConfig.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include "common/json.hpp"
#include "init.h"
using namespace std;
using json = nlohmann::json;

ArgsManager dCashArgs;
bool fPrintToConsole = false;
bool fPrintToDebugLog = true;

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
		//boost::thread_group threadGroup;
		//CScheduler scheduler;
		//bool fRet = false;

		setRelativePath(argv[0]);

		std::string mode("");
		mode += argv[1];

		if (mode != "mine" && mode != "scan") {
			LogPrintStr("Invalid mode");
			return(false);
		}

        CASH_TRY
        {
            dCashArgs.ReadConfigFile(argv[2]);
        } CASH_CATCH (const std::exception& e) {
            fprintf(stderr,"Error reading configuration file: %s\n", e.what());
            return false;
        }

		if (!AppInitBasicSetup(dCashArgs)) {
			LogPrintStr("Basic setup failed");
			return false;
		}
		if (!AppInitSanityChecks()) {
			LogPrintStr("Sanity checks failed");
			return false;
		}
		LogPrintStr("Sanity checks passed");
		std::string inRaw = ReadFile(argv[3]);
        std::string out = AppInitMain(inRaw, mode);

    	std::ofstream outFile(dCashArgs.GetPathArg("OUTPUT"));
    	if (outFile.is_open()) {
    		outFile << out;
    		outFile.close();
    	} else {
    		LogPrintStr("Failed to open output.");
    		return(false);
    	}

    	LogPrintStr("DevCash Shutting Down");
		return(true);
	} CASH_CATCH (...) {
		std::exception_ptr p = std::current_exception();
		std::string err("");
		err += (p ? p.__cxa_exception_type()->name() : "null");
		cout << "Error: "+err <<  std::endl;
		cerr << err << std::endl;
		return(false);
	}
}

int main(int argc, char* argv[])
{
	if (argc != 4) {
		std::cout << "Usage: DevCash mine|scan configfile input" << std::endl;
		return(EXIT_FAILURE);
	}

   return(AppInit(argc, argv) ? EXIT_SUCCESS : EXIT_FAILURE);
}
