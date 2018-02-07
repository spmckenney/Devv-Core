/*
 * init.cpp
 *
 *  Created on: Dec 10, 2017
 *      Author: Nick Williams
 */

#include "init.h"
#include <atomic>
#include <stdio.h>
#include <string>
#include <iostream>
#include "json.hpp"
using namespace std;
using json = nlohmann::json;

#include "consensus/statestub.h"
#include "consensus/validation.h"
#include <stdint.h>
#include <stdio.h>
#include <memory>
#include <csignal>

#ifndef WIN32
#include <signal.h>
#endif

/*#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/bind.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/thread.hpp>*/

bool fFeeEstimatesInitialized = false;
DCState myState();

#if (defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)) && not defined(DEVCASH_NOEXCEPTION)
    #define CASH_THROW(exception) throw exception
    #define CASH_TRY try
    #define CASH_CATCH(exception) catch(exception)
#else
    #define CASH_THROW(exception) std::abort()
    #define CASH_TRY if(true)
    #define CASH_CATCH(exception) if(false)
#endif

const char *DevCashContext::DEFAULT_PUBKEY = "-----BEGIN PUBLIC KEY-----MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4KLYaBof/CkaSGQrkmbbGr1C5pLqMXepNJFlqdS2mb3ZnB8Ut07VkFVyYSDS2Dj4AIWZE/JSKwsgTbFgipZ5NANffbOCzSRh+uWTklg6vtZm9jAZYypPVVfUYqxa+Q0dNtLotUW+IefHZ/D6AxuIRsRoZjvx/8Ir/BKoTZm/KuQBwSONWrM5aM6NPWJjUverMtLjwJTXo8VYBFTHfyxFJqjS9PMsjNKva3+nb/SCKoCR2xNxsQHheUSRv33t6XWb9UPfyFmKYjAfiOOZHS9/pABiY0/GsexrMsgDHKXa2mW3EK/z2IYq9sEqFf9YO8E8rWAXjQ58ZKf1yOnaDXcPMQIDAQAB-----END PUBLIC KEY-----";
const char *DevCashContext::DEFAULT_PK = "-----BEGIN PRIVATE KEY-----MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDgothoGh/8KRpIZCuSZtsavULmkuoxd6k0kWWp1LaZvdmcHxS3TtWQVXJhINLYOPgAhZkT8lIrCyBNsWCKlnk0A199s4LNJGH65ZOSWDq+1mb2MBljKk9VV9RirFr5DR020ui1Rb4h58dn8PoDG4hGxGhmO/H/wiv8EqhNmb8q5AHBI41aszlozo09YmNS96sy0uPAlNejxVgEVMd/LEUmqNL08yyM0q9rf6dv9IIqgJHbE3GxAeF5RJG/fe3pdZv1Q9/IWYpiMB+I45kdL3+kAGJjT8ax7GsyyAMcpdraZbcQr/PYhir2wSoV/1g7wTytYBeNDnxkp/XI6doNdw8xAgMBAAECggEAArYUdJU0I5//YDZNTFQPevAj2ZKWXwh5s1e56WXW2l4vPTIm1tuNulM9sSxrPw7Y93ClW1dGZJyaxDVK3AFa7yTHR0YeYwl4YUXaFR8ZfmoqDfigpdDB6l7IAnTgGDdvTdUX1/BCjjg08O04p0byyx/dvrYkgpi+XSmAfIdJhmP6U4Q5iBXiiBMwVDCG/sqC4QWX40LKCcd6NGJW5/RHJInoSAw2rhZOf9ZiCdHkHDvkjsKXkkMJ+LyO062lI1pXykhWmMe+qqaK/3xk+za9MhW7NZEsOiqtCk3PE9c27kybmFqXXRqhRLPZ4vr3M0zWUx4B6zn5ihssyyawPBQsAQKBgQDwI3NzdUf7VUhjzLhfGL4iKXfJR29zs3h2CVH7ZcGpHhTyE+R+a/vfkR2LXR/n/HynQlYZhZ8mm68Ynjpg9UqlA/B4Z/Ap6gVGTEPC3zU1jJox2qSle5aAndO2V/IyUM+n4vCeQ8jZzE0WYtAv73H5se+jtAwl0BCMNeKbWVKAcQKBgQDveUMEwlS2KPmSLWSykR7Hpn4V/UvOjmTWX8+RbNs0hxrlGduiDmVZB7smsCSQbhhfixkmsx4K4KYewfXvqjjVupbCmuFNkf2nECiV9t4ACNvmUtIIgXOf08oaZM895K24IbZdH5VjgXljXsMAulXbG7Ij1SjtXvWf1ykaNSTawQKBgCn9yP5zj7a/Xv00mzjl1rmajru/phmRVIsvbgqL7KVqATejit0gfNbHRWdNTXr/h7ynuO6VkxLpPmELqiGyQu9AFRi49CIgLfPw+hhld6R5ha0aEphtWA/9iTvlfRCXWPh+kpzaNZEATKqRdN4s/L0xBDqYDVe/XmVmNs37fJXBAoGASF1AX0PKDXG8WOvWrg8kWfh5yXNNYRGubwls0+ktJGZfPjPeJs5q2ch4SWyY3/wk6VpDM2qU/Xx9NnYuN0oc+pjzzcK3qpUfLUi4uvhqhWAn8yW7yk4z/mwlemxUI8Piqu2lCebtYbBSWjDchG/KWfe4kRNs1q4HU1HVXdIJXQECgYEAqqhFUHuJT7x2pIYiRsi4LTlJSKRUjN0pMqd2Hh6xlgS7VgiF32l5E+1SLTlKrcyXCZjcIaJrWWK9TQHXLCa+vIi2thcA8BGrdNqVwnXSPVX/o038cefP/Aj/iBGEMazp/olOidDG35EJp1gy00QBjCPDf5/svY3ed1ACw9fZ7k4=-----END PRIVATE KEY-----";
std::string pubKeyStr = "";
std::string pkStr = "-----BEGIN ENCRYPTED PRIVATE KEY-----\nMIHeMEkGCSqGSIb3DQEFDTA8MBsGCSqGSIb3DQEFDDAOBAgMnJzrxK/HEAICCAAw\nHQYJYIZIAWUDBAECBBD5kJEZMeZ2JBaiUB1RIdFRBIGQO8Z8P7ktVRBzGR5bUX8I\nXHG3dTLg+WNvU+C8ecFhSOStgpvhAzUPmX/KPJ1vrMQ07Qw0EGxR+TZ1bqUnMMRl\n6WhU/cVHBrThOwrk2Wx80CSc7uzl9UsZKtXwsR7/OXan5cyMxnHo3OVfA/0Vrrhk\n1CBBQjC22Pxw6lVlUijtPrR0sxhHruFSCZoMMu0NF0SK\n-----END ENCRYPTED PRIVATE KEY-----";

#ifdef WIN32
// Win32 LevelDB doesn't use filedescriptors, and the ones used for
// accessing block files don't count towards the fd_set size limit
// anyway.
#define MIN_CORE_FILEDESCRIPTORS 0
#else
#define MIN_CORE_FILEDESCRIPTORS 150
#endif

//////////////////////////////////////////////////////////////////////////////
//
// Shutdown
//

//
// Thread management and startup/shutdown:
//
// The network-processing threads are all part of a thread group
// created by AppInit()
//
// A clean exit happens when StartShutdown() or the SIGTERM
// signal handler sets fRequestShutdown, which makes main thread's
// WaitForShutdown() interrupts the thread group.
// And then, WaitForShutdown() makes all other on-going threads
// in the thread group join the main thread.
// Shutdown() is then called to clean up database connections, and stop other
// threads that should only be stopped after the main network-processing
// threads have exited.
//

std::atomic<bool> fRequestShutdown(false);
std::atomic<bool> fDumpMempoolLater(false);

void StartShutdown()
{
    fRequestShutdown = true;
}
bool ShutdownRequested()
{
    return fRequestShutdown;
}

/**
 * This is a minimally invasive approach to shutdown on LevelDB read errors from the
 * chainstate, while keeping user interface out of the common library
*/
/*class CCoinsViewErrorCatcher final : public CCoinsViewBacked
{
public:
    explicit CCoinsViewErrorCatcher(CCoinsView* view) : CCoinsViewBacked(view) {}
    bool GetCoin(const COutPoint &outpoint, Coin &coin) const override {
        try {
            return CCoinsViewBacked::GetCoin(outpoint, coin);
        } catch(const std::runtime_error& e) {
            uiInterface.ThreadSafeMessageBox(_("Error reading from database, shutting down."), "", CClientUIInterface::MSG_ERROR);
            LogPrintf("Error reading from database: %s\n", e.what());
            // Starting the shutdown sequence and returning false to the caller would be
            // interpreted as 'entry not found' (as opposed to unable to read data), and
            // could lead to invalid interpretation. Just exit immediately, as we can't
            // continue anyway, and all writes should be atomic.
            abort();
        }
    }
    // Writes do not need similar protection, as failure to write is handled by the caller.
};*/

//static std::unique_ptr<CCoinsViewErrorCatcher> pcoinscatcher;
//static std::unique_ptr<ECCVerifyHandle> globalVerifyHandle;

/*void Interrupt(boost::thread_group& threadGroup)
{
    InterruptHTTPServer();
    InterruptHTTPRPC();
    InterruptRPC();
    InterruptREST();
    InterruptTorControl();
    if (g_connman)
        g_connman->Interrupt();
    threadGroup.interrupt_all();
}*/

void Shutdown()
{
	LogPrintStr("Shutting down DevCash");

	static CCriticalSection cs_Shutdown;
	TRY_LOCK(cs_Shutdown, lockShutdown);
	if (!lockShutdown) return;

	try {
		RenameThread("shutdown");
	} catch (const std::exception& e) {
		FormatException(&e, "main.shutdown", DCLog::INIT);
	}

    /*
    /// Note: Shutdown() must be able to handle cases in which initialization failed part of the way,
    /// for example if the data directory was found to be locked.
    /// Be sure that anything that writes files or flushes caches only does this if the respective
    /// module was initialized.

    mempool.AddTransactionsUpdated(1);

    StopHTTPRPC();
    StopREST();
    StopRPC();
    StopHTTPServer();
    MapPort(false);

    // Because these depend on each-other, we make sure that neither can be
    // using the other before destroying them.
    if (peerLogic) UnregisterValidationInterface(peerLogic.get());
    if (g_connman) g_connman->Stop();
    peerLogic.reset();
    g_connman.reset();

    StopTorControl();
    if (fDumpMempoolLater && gArgs.GetArg("-persistmempool", DEFAULT_PERSIST_MEMPOOL)) {
        DumpMempool();
    }

    if (fFeeEstimatesInitialized)
    {
        ::feeEstimator.FlushUnconfirmed(::mempool);
        fs::path est_path = GetDataDir() / FEE_ESTIMATES_FILENAME;
        CAutoFile est_fileout(fsbridge::fopen(est_path, "wb"), SER_DISK, CLIENT_VERSION);
        if (!est_fileout.IsNull())
            ::feeEstimator.Write(est_fileout);
        else
            LogPrintf("%s: Failed to write fee estimates to %s\n", __func__, est_path.string());
        fFeeEstimatesInitialized = false;
    }

    // FlushStateToDisk generates a SetBestChain callback, which we should avoid missing
    if (pcoinsTip != nullptr) {
        FlushStateToDisk();
    }

    // After there are no more peers/RPC left to give us new data which may generate
    // CValidationInterface callbacks, flush them...
    GetMainSignals().FlushBackgroundCallbacks();

    // Any future callbacks will be dropped. This should absolutely be safe - if
    // missing a callback results in an unrecoverable situation, unclean shutdown
    // would too. The only reason to do the above flushes is to let the wallet catch
    // up with our current chain to avoid any strange pruning edge cases and make
    // next startup faster by avoiding rescan.

    {
        LOCK(cs_main);
        if (pcoinsTip != nullptr) {
            FlushStateToDisk();
        }
        pcoinsTip.reset();
        pcoinscatcher.reset();
        pcoinsdbview.reset();
        pblocktree.reset();
    }

    UnregisterAllValidationInterfaces();
    GetMainSignals().UnregisterBackgroundSignalScheduler();
    GetMainSignals().UnregisterWithMempoolSignals(mempool);
    globalVerifyHandle.reset();
    ECC_Stop();
    */
}

/*static void BlockNotifyCallback(bool initialSync, const CBlockIndex *pBlockIndex)
{
    if (initialSync || !pBlockIndex)
        return;

    std::string strCmd = gArgs.GetArg("-blocknotify", "");
    if (!strCmd.empty()) {
        boost::replace_all(strCmd, "%s", pBlockIndex->GetBlockHash().GetHex());
        boost::thread t(runCommand, strCmd); // thread runs free
    }
}*/

//static bool fHaveGenesis = false;
//static CWaitableCriticalSection cs_GenesisWait;
//static CConditionVariable condvar_GenesisWait;

/*static void BlockNotifyGenesisWait(bool, const CBlockIndex *pBlockIndex)
{
    if (pBlockIndex != nullptr) {
        {
            WaitableLock lock_GenesisWait(cs_GenesisWait);
            fHaveGenesis = true;
        }
        condvar_GenesisWait.notify_all();
    }
}

struct CImportingNow
{
    CImportingNow() {
        assert(fImporting == false);
        fImporting = true;
    }

    ~CImportingNow() {
        assert(fImporting == true);
        fImporting = false;
    }
};


// If we're using -prune with -reindex, then delete block files that will be ignored by the
// reindex.  Since reindexing works by starting at block file 0 and looping until a blockfile
// is missing, do the same here to delete any later block files after a gap.  Also delete all
// rev files since they'll be rewritten by the reindex anyway.  This ensures that vinfoBlockFile
// is in sync with what's actually on disk by the time we start downloading, so that pruning
// works correctly.
void CleanupBlockRevFiles()
{
    std::map<std::string, fs::path> mapBlockFiles;

    // Glob all blk?????.dat and rev?????.dat files from the blocks directory.
    // Remove the rev files immediately and insert the blk file paths into an
    // ordered map keyed by block file index.
    LogPrintf("Removing unusable blk?????.dat and rev?????.dat files for -reindex with -prune\n");
    fs::path blocksdir = GetDataDir() / "blocks";
    for (fs::directory_iterator it(blocksdir); it != fs::directory_iterator(); it++) {
        if (fs::is_regular_file(*it) &&
            it->path().filename().string().length() == 12 &&
            it->path().filename().string().substr(8,4) == ".dat")
        {
            if (it->path().filename().string().substr(0,3) == "blk")
                mapBlockFiles[it->path().filename().string().substr(3,5)] = it->path();
            else if (it->path().filename().string().substr(0,3) == "rev")
                remove(it->path());
        }
    }

    // Remove all block files that aren't part of a contiguous set starting at
    // zero by walking the ordered map (keys are block file indices) by
    // keeping a separate counter.  Once we hit a gap (or if 0 doesn't exist)
    // start removing block files.
    int nContigCounter = 0;
    for (const std::pair<std::string, fs::path>& item : mapBlockFiles) {
        if (atoi(item.first) == nContigCounter) {
            nContigCounter++;
            continue;
        }
        remove(item.second);
    }
}

void ThreadImport(std::vector<fs::path> vImportFiles)
{
    const CChainParams& chainparams = Params();
    RenameThread("bitcoin-loadblk");

    {
    CImportingNow imp;

    // -reindex
    if (fReindex) {
        int nFile = 0;
        while (true) {
            CDiskBlockPos pos(nFile, 0);
            if (!fs::exists(GetBlockPosFilename(pos, "blk")))
                break; // No block files left to reindex
            FILE *file = OpenBlockFile(pos, true);
            if (!file)
                break; // This error is logged in OpenBlockFile
            LogPrintf("Reindexing block file blk%05u.dat...\n", (unsigned int)nFile);
            LoadExternalBlockFile(chainparams, file, &pos);
            nFile++;
        }
        pblocktree->WriteReindexing(false);
        fReindex = false;
        LogPrintf("Reindexing finished\n");
        // To avoid ending up in a situation without genesis block, re-try initializing (no-op if reindexing worked):
        LoadGenesisBlock(chainparams);
    }

    // hardcoded $DATADIR/bootstrap.dat
    fs::path pathBootstrap = GetDataDir() / "bootstrap.dat";
    if (fs::exists(pathBootstrap)) {
        FILE *file = fsbridge::fopen(pathBootstrap, "rb");
        if (file) {
            fs::path pathBootstrapOld = GetDataDir() / "bootstrap.dat.old";
            LogPrintf("Importing bootstrap.dat...\n");
            LoadExternalBlockFile(chainparams, file);
            RenameOver(pathBootstrap, pathBootstrapOld);
        } else {
            LogPrintf("Warning: Could not open bootstrap file %s\n", pathBootstrap.string());
        }
    }

    // -loadblock=
    for (const fs::path& path : vImportFiles) {
        FILE *file = fsbridge::fopen(path, "rb");
        if (file) {
            LogPrintf("Importing blocks file %s...\n", path.string());
            LoadExternalBlockFile(chainparams, file);
        } else {
            LogPrintf("Warning: Could not open blocks file %s\n", path.string());
        }
    }

    // scan for better chains in the block chain database, that are not yet connected in the active best chain
    CValidationState state;
    if (!ActivateBestChain(state, chainparams)) {
        LogPrintf("Failed to connect best block");
        StartShutdown();
    }

    if (gArgs.GetBoolArg("-stopafterblockimport", DEFAULT_STOPAFTERBLOCKIMPORT)) {
        LogPrintf("Stopping after block import\n");
        StartShutdown();
    }
    } // End scope of CImportingNow
    if (gArgs.GetArg("-persistmempool", DEFAULT_PERSIST_MEMPOOL)) {
        LoadMempool();
        fDumpMempoolLater = !fRequestShutdown;
    }
}*/

void InitError(const std::exception* e)
{
	FormatException(e, "main", DCLog::INIT);
}

namespace { // Variables internal to initialization process only

//int nMaxConnections;
//int nUserMaxConnections;
//int nFD;

} // namespace

bool AppInitBasicSetup(ArgsManager &args)
{
#ifdef _MSC_VER
    // Turn off Microsoft heap dump noise
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, CreateFileA("NUL", GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, 0));
    // Disable confusing "helpful" text message on abort, Ctrl-C
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
#ifdef WIN32
    // Enable Data Execution Prevention (DEP)
    // Minimum supported OS versions: WinXP SP3, WinVista >= SP1, Win Server 2008
    // A failure is non-critical and needs no further attention!
#ifndef PROCESS_DEP_ENABLE
    // We define this here, because GCCs winbase.h limits this to _WIN32_WINNT >= 0x0601 (Windows 7),
    // which is not correct. Can be removed, when GCCs winbase.h is fixed!
#define PROCESS_DEP_ENABLE 0x00000001
#endif
    typedef BOOL (WINAPI *PSETPROCDEPPOL)(DWORD);
    PSETPROCDEPPOL setProcDEPPol = (PSETPROCDEPPOL)GetProcAddress(GetModuleHandleA("Kernel32.dll"), "SetProcessDEPPolicy");
    if (setProcDEPPol != nullptr) setProcDEPPol(PROCESS_DEP_ENABLE);
#endif

    if (!SetupNetworking()) return LogPrintStr("Initializing networking failed");

#ifndef WIN32

    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGHUP, signalHandler);

    // Ignore SIGPIPE, otherwise it will bring the daemon down if the client closes unexpectedly
    signal(SIGPIPE, SIG_IGN);
#endif

    CASH_TRY {
		OpenSSL_add_all_algorithms();
		ERR_load_crypto_strings();
		pubKeyStr = args.GetArg("PUBKEY", DevCashContext::DEFAULT_PUBKEY);
		LogPrintStr("Public Key: "+pubKeyStr);
		//pkStr = args.GetArg("PRIVATE", DevCashContext::DEFAULT_PK);
		LogPrintStr("Private Key: "+pkStr);
	} CASH_CATCH (const std::exception& e) {
		InitError(&e);
	}

    return true;
}

/*bool AppInitParameterInteraction()
{
    /*const CChainParams& chainparams = Params();
    // ********************************************************* Step 2: parameter interactions

    // also see: InitParameterInteraction()

    // if using block pruning, then disallow txindex
    if (gArgs.GetArg("-prune", 0)) {
        if (gArgs.GetBoolArg("-txindex", DEFAULT_TXINDEX))
            return InitError(_("Prune mode is incompatible with -txindex."));
    }

    // -bind and -whitebind can't be set when not listening
    size_t nUserBind = gArgs.GetArgs("-bind").size() + gArgs.GetArgs("-whitebind").size();
    if (nUserBind != 0 && !gArgs.GetBoolArg("-listen", DEFAULT_LISTEN)) {
        return InitError("Cannot set -bind or -whitebind together with -listen=0");
    }

    // Make sure enough file descriptors are available
    int nBind = std::max(nUserBind, size_t(1));
    nUserMaxConnections = gArgs.GetArg("-maxconnections", DEFAULT_MAX_PEER_CONNECTIONS);
    nMaxConnections = std::max(nUserMaxConnections, 0);

    // Trim requested connection counts, to fit into system limitations
    nMaxConnections = std::max(std::min(nMaxConnections, (int)(FD_SETSIZE - nBind - MIN_CORE_FILEDESCRIPTORS - MAX_ADDNODE_CONNECTIONS)), 0);
    nFD = RaiseFileDescriptorLimit(nMaxConnections + MIN_CORE_FILEDESCRIPTORS + MAX_ADDNODE_CONNECTIONS);
    if (nFD < MIN_CORE_FILEDESCRIPTORS)
        return InitError(_("Not enough file descriptors available."));
    nMaxConnections = std::min(nFD - MIN_CORE_FILEDESCRIPTORS - MAX_ADDNODE_CONNECTIONS, nMaxConnections);

    if (nMaxConnections < nUserMaxConnections)
        InitWarning(strprintf(_("Reducing -maxconnections from %d to %d, because of system limitations."), nUserMaxConnections, nMaxConnections));

    // ********************************************************* Step 3: parameter-to-internal-flags
    if (gArgs.IsArgSet("-debug")) {
        // Special-case: if -debug=0/-nodebug is set, turn off debugging messages
        const std::vector<std::string> categories = gArgs.GetArgs("-debug");

        if (std::none_of(categories.begin(), categories.end(),
            [](std::string cat){return cat == "0" || cat == "none";})) {
            for (const auto& cat : categories) {
                uint32_t flag = 0;
                if (!GetLogCategory(&flag, &cat)) {
                    InitWarning(strprintf(_("Unsupported logging category %s=%s."), "-debug", cat));
                    continue;
                }
                logCategories |= flag;
            }
        }
    }

    // Now remove the logging categories which were explicitly excluded
    for (const std::string& cat : gArgs.GetArgs("-debugexclude")) {
        uint32_t flag = 0;
        if (!GetLogCategory(&flag, &cat)) {
            InitWarning(strprintf(_("Unsupported logging category %s=%s."), "-debugexclude", cat));
            continue;
        }
        logCategories &= ~flag;
    }

    // Check for -debugnet
    if (gArgs.GetBoolArg("-debugnet", false))
        InitWarning(_("Unsupported argument -debugnet ignored, use -debug=net."));
    // Check for -socks - as this is a privacy risk to continue, exit here
    if (gArgs.IsArgSet("-socks"))
        return InitError(_("Unsupported argument -socks found. Setting SOCKS version isn't possible anymore, only SOCKS5 proxies are supported."));
    // Check for -tor - as this is a privacy risk to continue, exit here
    if (gArgs.GetBoolArg("-tor", false))
        return InitError(_("Unsupported argument -tor found, use -onion."));

    if (gArgs.GetBoolArg("-benchmark", false))
        InitWarning(_("Unsupported argument -benchmark ignored, use -debug=bench."));

    if (gArgs.GetBoolArg("-whitelistalwaysrelay", false))
        InitWarning(_("Unsupported argument -whitelistalwaysrelay ignored, use -whitelistrelay and/or -whitelistforcerelay."));

    if (gArgs.IsArgSet("-blockminsize"))
        InitWarning("Unsupported argument -blockminsize ignored.");

    // Checkmempool and checkblockindex default to true in regtest mode
    int ratio = std::min<int>(std::max<int>(gArgs.GetArg("-checkmempool", chainparams.DefaultConsistencyChecks() ? 1 : 0), 0), 1000000);
    if (ratio != 0) {
        mempool.setSanityCheck(1.0 / ratio);
    }
    fCheckBlockIndex = gArgs.GetBoolArg("-checkblockindex", chainparams.DefaultConsistencyChecks());
    fCheckpointsEnabled = gArgs.GetBoolArg("-checkpoints", DEFAULT_CHECKPOINTS_ENABLED);

    hashAssumeValid = uint256S(gArgs.GetArg("-assumevalid", chainparams.GetConsensus().defaultAssumeValid.GetHex()));
    if (!hashAssumeValid.IsNull())
        LogPrintf("Assuming ancestors of block %s have valid signatures.\n", hashAssumeValid.GetHex());
    else
        LogPrintf("Validating signatures for all blocks.\n");

    if (gArgs.IsArgSet("-minimumchainwork")) {
        const std::string minChainWorkStr = gArgs.GetArg("-minimumchainwork", "");
        if (!IsHexNumber(minChainWorkStr)) {
            return InitError(strprintf("Invalid non-hex (%s) minimum chain work value specified", minChainWorkStr));
        }
        nMinimumChainWork = UintToArith256(uint256S(minChainWorkStr));
    } else {
        nMinimumChainWork = UintToArith256(chainparams.GetConsensus().nMinimumChainWork);
    }
    LogPrintf("Setting nMinimumChainWork=%s\n", nMinimumChainWork.GetHex());
    if (nMinimumChainWork < UintToArith256(chainparams.GetConsensus().nMinimumChainWork)) {
        LogPrintf("Warning: nMinimumChainWork set below default value of %s\n", chainparams.GetConsensus().nMinimumChainWork.GetHex());
    }

    // mempool limits
    int64_t nMempoolSizeMax = gArgs.GetArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000;
    int64_t nMempoolSizeMin = gArgs.GetArg("-limitdescendantsize", DEFAULT_DESCENDANT_SIZE_LIMIT) * 1000 * 40;
    if (nMempoolSizeMax < 0 || nMempoolSizeMax < nMempoolSizeMin)
        return InitError(strprintf(_("-maxmempool must be at least %d MB"), std::ceil(nMempoolSizeMin / 1000000.0)));
    // incremental relay fee sets the minimum feerate increase necessary for BIP 125 replacement in the mempool
    // and the amount the mempool min fee increases above the feerate of txs evicted due to mempool limiting.
    if (gArgs.IsArgSet("-incrementalrelayfee"))
    {
        CAmount n = 0;
        if (!ParseMoney(gArgs.GetArg("-incrementalrelayfee", ""), n))
            return InitError(AmountErrMsg("incrementalrelayfee", gArgs.GetArg("-incrementalrelayfee", "")));
        incrementalRelayFee = CFeeRate(n);
    }

    // -par=0 means autodetect, but nScriptCheckThreads==0 means no concurrency
    nScriptCheckThreads = gArgs.GetArg("-par", DEFAULT_SCRIPTCHECK_THREADS);
    if (nScriptCheckThreads <= 0)
        nScriptCheckThreads += GetNumCores();
    if (nScriptCheckThreads <= 1)
        nScriptCheckThreads = 0;
    else if (nScriptCheckThreads > MAX_SCRIPTCHECK_THREADS)
        nScriptCheckThreads = MAX_SCRIPTCHECK_THREADS;

    // block pruning; get the amount of disk space (in MiB) to allot for block & undo files
    int64_t nPruneArg = gArgs.GetArg("-prune", 0);
    if (nPruneArg < 0) {
        return InitError(_("Prune cannot be configured with a negative value."));
    }
    nPruneTarget = (uint64_t) nPruneArg * 1024 * 1024;
    if (nPruneArg == 1) {  // manual pruning: -prune=1
        LogPrintf("Block pruning enabled.  Use RPC call pruneblockchain(height) to manually prune block and undo files.\n");
        nPruneTarget = std::numeric_limits<uint64_t>::max();
        fPruneMode = true;
    } else if (nPruneTarget) {
        if (nPruneTarget < MIN_DISK_SPACE_FOR_BLOCK_FILES) {
            return InitError(strprintf(_("Prune configured below the minimum of %d MiB.  Please use a higher number."), MIN_DISK_SPACE_FOR_BLOCK_FILES / 1024 / 1024));
        }
        LogPrintf("Prune configured to target %uMiB on disk for block and undo files.\n", nPruneTarget / 1024 / 1024);
        fPruneMode = true;
    }

    nConnectTimeout = gArgs.GetArg("-timeout", DEFAULT_CONNECT_TIMEOUT);
    if (nConnectTimeout <= 0)
        nConnectTimeout = DEFAULT_CONNECT_TIMEOUT;

    if (gArgs.IsArgSet("-minrelaytxfee")) {
        CAmount n = 0;
        if (!ParseMoney(gArgs.GetArg("-minrelaytxfee", ""), n)) {
            return InitError(AmountErrMsg("minrelaytxfee", gArgs.GetArg("-minrelaytxfee", "")));
        }
        // High fee check is done afterward in WalletParameterInteraction()
        ::minRelayTxFee = CFeeRate(n);
    } else if (incrementalRelayFee > ::minRelayTxFee) {
        // Allow only setting incrementalRelayFee to control both
        ::minRelayTxFee = incrementalRelayFee;
        LogPrintf("Increasing minrelaytxfee to %s to match incrementalrelayfee\n",::minRelayTxFee.ToString());
    }

    // Sanity check argument for min fee for including tx in block
    // TODO: Harmonize which arguments need sanity checking and where that happens
    if (gArgs.IsArgSet("-blockmintxfee"))
    {
        CAmount n = 0;
        if (!ParseMoney(gArgs.GetArg("-blockmintxfee", ""), n))
            return InitError(AmountErrMsg("blockmintxfee", gArgs.GetArg("-blockmintxfee", "")));
    }

    // Feerate used to define dust.  Shouldn't be changed lightly as old
    // implementations may inadvertently create non-standard transactions
    if (gArgs.IsArgSet("-dustrelayfee"))
    {
        CAmount n = 0;
        if (!ParseMoney(gArgs.GetArg("-dustrelayfee", ""), n) || 0 == n)
            return InitError(AmountErrMsg("dustrelayfee", gArgs.GetArg("-dustrelayfee", "")));
        dustRelayFee = CFeeRate(n);
    }

    fRequireStandard = !gArgs.GetBoolArg("-acceptnonstdtxn", !chainparams.RequireStandard());
    if (chainparams.RequireStandard() && !fRequireStandard)
        return InitError(strprintf("acceptnonstdtxn is not currently supported for %s chain", chainparams.NetworkIDString()));
    nBytesPerSigOp = gArgs.GetArg("-bytespersigop", nBytesPerSigOp);

#ifdef ENABLE_WALLET
    if (!WalletParameterInteraction())
        return false;
#endif

    fIsBareMultisigStd = gArgs.GetBoolArg("-permitbaremultisig", DEFAULT_PERMIT_BAREMULTISIG);
    fAcceptDatacarrier = gArgs.GetBoolArg("-datacarrier", DEFAULT_ACCEPT_DATACARRIER);
    nMaxDatacarrierBytes = gArgs.GetArg("-datacarriersize", nMaxDatacarrierBytes);

    // Option to startup with mocktime set (used for regression testing):
    SetMockTime(gArgs.GetArg("-mocktime", 0)); // SetMockTime(0) is a no-op

    if (gArgs.GetBoolArg("-peerbloomfilters", DEFAULT_PEERBLOOMFILTERS))
        nLocalServices = ServiceFlags(nLocalServices | NODE_BLOOM);

    if (gArgs.GetArg("-rpcserialversion", DEFAULT_RPC_SERIALIZE_VERSION) < 0)
        return InitError("rpcserialversion must be non-negative.");

    if (gArgs.GetArg("-rpcserialversion", DEFAULT_RPC_SERIALIZE_VERSION) > 1)
        return InitError("unknown rpcserialversion requested.");

    nMaxTipAge = gArgs.GetArg("-maxtipage", DEFAULT_MAX_TIP_AGE);

    fEnableReplacement = gArgs.GetBoolArg("-mempoolreplacement", DEFAULT_ENABLE_REPLACEMENT);
    if ((!fEnableReplacement) && gArgs.IsArgSet("-mempoolreplacement")) {
        // Minimal effort at forwards compatibility
        std::string strReplacementModeList = gArgs.GetArg("-mempoolreplacement", "");  // default is impossible
        std::vector<std::string> vstrReplacementModes;
        boost::split(vstrReplacementModes, strReplacementModeList, boost::is_any_of(","));
        fEnableReplacement = (std::find(vstrReplacementModes.begin(), vstrReplacementModes.end(), "fee") != vstrReplacementModes.end());
    }

    if (gArgs.IsArgSet("-vbparams")) {
        // Allow overriding version bits parameters for testing
        if (!chainparams.MineBlocksOnDemand()) {
            return InitError("Version bits parameters may only be overridden on regtest.");
        }
        for (const std::string& strDeployment : gArgs.GetArgs("-vbparams")) {
            std::vector<std::string> vDeploymentParams;
            boost::split(vDeploymentParams, strDeployment, boost::is_any_of(":"));
            if (vDeploymentParams.size() != 3) {
                return InitError("Version bits parameters malformed, expecting deployment:start:end");
            }
            int64_t nStartTime, nTimeout;
            if (!ParseInt64(vDeploymentParams[1], &nStartTime)) {
                return InitError(strprintf("Invalid nStartTime (%s)", vDeploymentParams[1]));
            }
            if (!ParseInt64(vDeploymentParams[2], &nTimeout)) {
                return InitError(strprintf("Invalid nTimeout (%s)", vDeploymentParams[2]));
            }
            bool found = false;
            for (int j=0; j<(int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; ++j)
            {
                if (vDeploymentParams[0].compare(VersionBitsDeploymentInfo[j].name) == 0) {
                    UpdateVersionBitsParameters(Consensus::DeploymentPos(j), nStartTime, nTimeout);
                    found = true;
                    LogPrintf("Setting version bits activation parameters for %s to start=%ld, timeout=%ld\n", vDeploymentParams[0], nStartTime, nTimeout);
                    break;
                }
            }
            if (!found) {
                return InitError(strprintf("Invalid deployment (%s)", vDeploymentParams[0]));
            }
        }
    }
    return true;
}*/

bool AppInitSanityChecks()
{
	bool retVal = false;
	CASH_TRY {
		EVP_MD_CTX *ctx;
		if(!(ctx = EVP_MD_CTX_create())) LogPrintStr("Could not create signature context!");

		std::string msg("hello");
		char hash[SHA256_DIGEST_LENGTH*2+1];
		dcHash(msg, hash);

		/*std::string xfer("\"addr\":0,\"amount\":-10,\"nonce\":1517434459");
		char tempHash[SHA256_DIGEST_LENGTH*2+1];
		dcHash(xfer, tempHash);*/

		EC_KEY* loadkey = loadEcKey(ctx, pubKeyStr, pkStr);

		//std::string testSign = sign(loadkey, xfer);
		//std::cout << testSign << std::endl;

		/*EC_KEY* eckey = genEcKey(ctx, pubKeyStr, pkStr);
		std::string tempSign = sign(eckey, xfer);*/

		std::string sDer = sign(loadkey, msg);
		return(verifySig(loadkey, msg, sDer));
	} CASH_CATCH (const std::exception& e) {
		InitError(&e);
	}
    return(retVal);
}

static std::vector<uint8_t> hex2CBOR(std::string hex) {
	int len = hex.length();
	std::vector<uint8_t> buf(len/2+1);
	for (int i=0;i<len/2;i++) {
		buf.at(i) = (uint8_t) char2int(hex.at(i*2))*16+char2int(hex.at(1+i*2));
	}
	buf.pop_back(); //remove null terminator
	return(buf);
}

static std::string CBOR2hex(std::vector<uint8_t> b) {
	int len = b.size();
	std::stringstream ss;
	for (int j=0; j<len; j++) {
		int c = (int) b[j];
		ss.put(alpha[(c>>4)&0xF]);
		ss.put(alpha[c&0xF]);
	}
	return(ss.str());
}

std::string AppInitMain(std::string inStr, std::string mode)
{
	std::string out("");
	CASH_TRY {
		if (mode == "mine") {
			LogPrintStr("Miner Mode");
			json j = json::parse(inStr);
			int txCnt = 0;
			if (j.is_object()) {
				txCnt = 1;
			} else if (j.is_array()) {
				txCnt = j.size();
			}
			std::vector<DCTransaction> txs;
			DCValidationBlock vBlock;
			EVP_MD_CTX *ctx;
			if(!(ctx = EVP_MD_CTX_create())) LogPrintStr("Could not create signature context!");
			if (txCnt < 1) {
				LogPrintStr("No transactions.  Nothing to do.");
				return("");
			} else if (txCnt == 1) {
				LogPrintStr("Single transaction");
				DCTransaction* tx = new DCTransaction(inStr);
				txs.push_back(*tx);
			} else {
				LogPrintStr(j[TX_TAG].size()+" transactions");
				for (auto iter = j[TX_TAG].begin(); iter != j[TX_TAG].end(); ++iter) {
					std::string tx = iter.value().dump();
					DCTransaction* t = new DCTransaction(tx);
					txs.push_back(*t);
				}
			}
			EC_KEY* eckey = loadEcKey(ctx, pubKeyStr, pkStr);
			DCBlock newBlock = DCBlock(txs, vBlock);
			if (newBlock.validate(eckey)) {
				newBlock.signBlock(eckey);
				newBlock.finalize();
				LogPrintStr("Output block");
				out = newBlock.ToCBOR_str();
			} else {
				LogPrintStr("No valid transactions");
			}

		} else if (mode == "scan") {
			LogPrintStr("Scanner Mode");
			std::vector<uint8_t> buffer = hex2CBOR(out);
			json j = json::from_cbor(buffer);
			out = j.dump();
			out.erase(remove(out.begin(), out.end(), '\\'), out.end()); //remove escape chars
		}
	} CASH_CATCH (const std::exception& e) {
		InitError(&e);
	}
    return out;
}
