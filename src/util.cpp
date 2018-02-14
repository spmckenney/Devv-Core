/*
 * util.cpp
 *
 *  Created on: Dec 27, 2017
 *      Author: Nick Williams
 */

#include "init.h"
#include "util.h"
#include "sync.h"
#include "utilstrencodings.h"
#include "utiltime.h"

#include <iostream>
#include <fstream>
#include <stdarg.h>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <locale>
#include <unistd.h>

#if (defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__))
#include <pthread.h>
#include <pthread_np.h>
#endif

#ifndef WIN32
// for posix_fallocate
#ifdef __linux__

#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif

#define _POSIX_C_SOURCE 200112L

#endif // __linux__

#include <algorithm>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h>

#else

#ifdef _MSC_VER
#pragma warning(disable:4786)
#pragma warning(disable:4804)
#pragma warning(disable:4805)
#pragma warning(disable:4717)
#endif

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0501

#ifdef _WIN32_IE
#undef _WIN32_IE
#endif
#define _WIN32_IE 0x0501

#define WIN32_LEAN_AND_MEAN 1
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <io.h> /* for _commit */
#include <shlobj.h>
#endif

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

#ifdef HAVE_MALLOPT_ARENA_MAX
#include <malloc.h>
#endif

#include <openssl/crypto.h>
#include <openssl/rand.h>
#include <openssl/conf.h>
using namespace std;

// Application startup time (used for uptime calculation)
const int64_t nStartupTime = GetTime();

std::string relativePath("");
std::string debugLogPath("");

bool fLogTimestamps = DEFAULT_LOGTIMESTAMPS;
bool fLogTimeMicros = DEFAULT_LOGTIMEMICROS;
bool fLogIPs = DEFAULT_LOGIPS;
std::atomic<bool> fReopenDebugLog(false);

/** Log categories bitfield. */
std::atomic<uint32_t> logCategories(0);

// Singleton for wrapping OpenSSL setup/teardown.
class CInit
{
public:
    CInit()
    {

        // OpenSSL can optionally load a config file which lists optional loadable modules and engines.
        // We don't use them so we don't require the config. However some of our libs may call functions
        // which attempt to load the config file, possibly resulting in an exit() or crash if it is missing
        // or corrupt. Explicitly tell OpenSSL not to try to load the file. The result for our libs will be
        // that the config appears to have been loaded and there are no modules/engines available.
        //OPENSSL_no_config();
    }
}
instance_of_cinit;

std::ostream* fileout = &std::cout;

int FileWriteStr(const std::string &str, std::ostream* fout)
{
	*fout << str << std::endl;
    return sizeof(str);
}

std::string ReadFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.good()) LogPrintStr("File "+filePath+" could not be found");
    if (!file.is_open()) LogPrintStr("File "+filePath+" could not be opened, check permissions");

    std::string output("");
	for(std::string line; std::getline(file, line);) {
		output += line;
	}
	return(output);
}

bool OpenDebugLog(std::string logPath)
{
	trim(logPath);
	fileout = new std::ofstream(logPath, std::fstream::app);
	cout << "Check DevCash logs at "+logPath << std::endl;
	LogPrintStr("DevCash initializing...");
    return true;
}

struct CLogCategoryDesc
{
    uint32_t flag;
    std::string category;
};

const CLogCategoryDesc LogCategories[] =
{
    {DCLog::NONE, "0"},
    {DCLog::NONE, "none"},
	{DCLog::INIT, "init"},
    {DCLog::NET, "net"},
    {DCLog::MEMPOOL, "mempool"},
    {DCLog::HTTP, "http"},
    {DCLog::BENCH, "bench"},
    {DCLog::DB, "db"},
    {DCLog::ESTIMATEFEE, "estimatefee"},
    {DCLog::ADDRMAN, "addrman"},
    {DCLog::SELECTCOINS, "selectcoins"},
    {DCLog::REINDEX, "reindex"},
    {DCLog::CMPCTBLOCK, "cmpctblock"},
    {DCLog::CRYPTO, "crypto"},
    {DCLog::PRUNE, "prune"},
    {DCLog::PROXY, "proxy"},
    {DCLog::MEMPOOLREJ, "mempoolrej"},
    {DCLog::LIBEVENT, "libevent"},
    {DCLog::COINDB, "coindb"},
    {DCLog::LEVELDB, "leveldb"},
    {DCLog::ALL, "1"},
    {DCLog::ALL, "all"},
};

bool GetLogCategory(uint32_t *f, const std::string *str)
{
    if (f && str) {
        if (*str == "") {
            *f = DCLog::ALL;
            return true;
        }
        for (unsigned int i = 0; i < ARRAYLEN(LogCategories); i++) {
            if (LogCategories[i].category == *str) {
                *f = LogCategories[i].flag;
                return true;
            }
        }
    }
    return false;
}

std::string ListLogCategories()
{
    std::string ret;
    int outcount = 0;
    for (unsigned int i = 0; i < ARRAYLEN(LogCategories); i++) {
        // Omit the special cases.
        if (LogCategories[i].flag != DCLog::NONE && LogCategories[i].flag != DCLog::ALL) {
            if (outcount != 0) ret += ", ";
            ret += LogCategories[i].category;
            outcount++;
        }
    }
    return ret;
}

std::vector<CLogCategoryActive> ListActiveLogCategories()
{
    std::vector<CLogCategoryActive> ret;
    for (unsigned int i = 0; i < ARRAYLEN(LogCategories); i++) {
        // Omit the special cases.
        if (LogCategories[i].flag != DCLog::NONE && LogCategories[i].flag != DCLog::ALL) {
            CLogCategoryActive catActive;
            catActive.category = LogCategories[i].category;
            catActive.active = LogAcceptCategory(LogCategories[i].flag);
            ret.push_back(catActive);
        }
    }
    return ret;
}

/**
 * fStartedNewLine is a state variable held by the calling context that will
 * suppress printing of the timestamp when multiple calls are made that don't
 * end in a newline. Initialize it to true, and hold it, in the calling context.
 */
static std::string LogTimestampStr(const std::string &str, std::atomic_bool *fStartedNewLine)
{
    std::string strStamped;

    if (!fLogTimestamps)
        return str;

    //if (!*fStartedNewLine) cout << "No new line?" <<std::endl;

    if (*fStartedNewLine) {
    	auto curTime = std::chrono::system_clock::now();
    	std::time_t timeInt = std::chrono::system_clock::to_time_t(curTime);
    	strStamped = std::ctime(&timeInt);
        if (fLogTimeMicros) {
        	int64_t nTimeMicros = GetTimeMicros();
        	strStamped += strprintf(".%06d", nTimeMicros%1000000);
        }
        trim(strStamped);

        strStamped += ' ' + str;
    } else strStamped = str;

    return strStamped;
}

int LogPrintStr(const std::string &str)
{
    int ret = 0; // Returns total number of characters written
    static std::atomic_bool fStartedNewLine(true);

    std::string strTimestamped = LogTimestampStr(str, &fStartedNewLine);
    if (fPrintToConsole)
    {
        // print to console
        ret = fwrite(strTimestamped.data(), 1, strTimestamped.size(), stdout);
        fflush(stdout);
    }
    else if (fPrintToDebugLog)
    	ret = FileWriteStr(strTimestamped, fileout);
    return ret;
}

/** Interpret string as boolean, for argument parsing */
static bool InterpretBool(const std::string& strValue)
{
    if (strValue.empty())
        return true;
    return (atoi(strValue) != 0);
}

/** Turn -noX into -X=0 */
static void InterpretNegativeSetting(std::string& strKey, std::string& strValue)
{
    if (strKey.length()>3 && strKey[0]=='-' && strKey[1]=='n' && strKey[2]=='o')
    {
        strKey = "-" + strKey.substr(3);
        strValue = InterpretBool(strValue) ? "0" : "1";
    }
}

void ArgsManager::ParseParameters(int argc, const char* const argv[])
{
    LOCK(cs_args);
    mapArgs.clear();
    mapMultiArgs.clear();

    for (int i = 1; i < argc; i++)
    {
        std::string str(argv[i]);
        std::string strValue;
        size_t is_index = str.find('=');
        if (is_index != std::string::npos)
        {
            strValue = str.substr(is_index+1);
            str = str.substr(0, is_index);
        }
#ifdef WIN32
        boost::to_lower(str);
        if (boost::algorithm::starts_with(str, "/"))
            str = "-" + str.substr(1);
#endif

        if (str[0] != '-')
            break;

        // Interpret --foo as -foo.
        // If both --foo and -foo are set, the last takes effect.
        if (str.length() > 1 && str[1] == '-')
            str = str.substr(1);
        InterpretNegativeSetting(str, strValue);

        mapArgs[str] = strValue;
        mapMultiArgs[str].push_back(strValue);
    }
}

std::vector<std::string> ArgsManager::GetArgs(const std::string& strArg) const
{
    LOCK(cs_args);
    auto it = mapMultiArgs.find(strArg);
    if (it != mapMultiArgs.end()) return it->second;
    return {};
}

bool ArgsManager::IsArgSet(const std::string& strArg) const
{
    LOCK(cs_args);
    return mapArgs.count(strArg);
}

std::string ArgsManager::GetArg(const std::string& strArg, const std::string& strDefault) const
{
    LOCK(cs_args);
    auto it = mapArgs.find(strArg);
    if (it != mapArgs.end()) return it->second;
    return strDefault;
}

std::string ArgsManager::GetPathArg(const std::string& strArg) const
{
    std::string arg(GetArg(strArg, ""));
    return(getRelativePath()+arg);
}

int64_t ArgsManager::GetArg(const std::string& strArg, int64_t nDefault) const
{
    LOCK(cs_args);
    auto it = mapArgs.find(strArg);
    if (it != mapArgs.end()) return atoi64(it->second);
    return nDefault;
}

bool ArgsManager::GetBoolArg(const std::string& strArg, bool fDefault) const
{
    LOCK(cs_args);
    auto it = mapArgs.find(strArg);
    if (it != mapArgs.end()) return InterpretBool(it->second);
    return fDefault;
}

bool ArgsManager::SoftSetArg(const std::string& strArg, const std::string& strValue)
{
    LOCK(cs_args);
    if (IsArgSet(strArg)) return false;
    ForceSetArg(strArg, strValue);
    return true;
}

bool ArgsManager::SoftSetBoolArg(const std::string& strArg, bool fValue)
{
    if (fValue)
        return SoftSetArg(strArg, std::string("1"));
    else
        return SoftSetArg(strArg, std::string("0"));
}

void ArgsManager::ForceSetArg(const std::string& strArg, const std::string& strValue)
{
    LOCK(cs_args);
    mapArgs[strArg] = strValue;
    mapMultiArgs[strArg] = {strValue};
}

std::string FormatException(const std::exception* pex, const std::string pszThread, uint32_t catNum)
{
	CLogCategoryDesc module = LogCategories[catNum];
	std::string msg = "";
    if (pex) {
    	msg += "EXCEPTION[";
    	msg += pszThread;
    	msg += "][";
    	msg += module.category;
    	msg += "] ";
    	msg += std::string(typeid(*pex).name());
    	msg += ": ";
    	msg += std::string(pex->what());

    } else {
    	msg += "UNKNOWN EXCEPTION[";
    	msg += pszThread;
		msg += "][";
		msg += module.category;
		msg += "] ";
    }
    LogPrintStr(msg);
    return(msg);
}

void setRelativePath(std::string arg0) {
	relativePath = trim(arg0);
}
std::string getRelativePath() {
	return(relativePath);
}

static CCriticalSection csPathCached;

void ArgsManager::ReadConfigFile(const std::string& confPath)
{
    std::ifstream streamConfig(confPath);
    if (!streamConfig.good()) throw std::runtime_error("Config file could not be found");
    if (!streamConfig.is_open()) throw std::runtime_error("Config file could not be opened, check permissions");

	LOCK(cs_args);
	for(std::string line; std::getline(streamConfig, line);) {
		string::size_type eq = line.find("=");
		if (eq != string::npos) {
			std::string key = line.substr(0, eq);
			std::string val = line.substr(eq+1);
			if ("RELPATH" == key) setRelativePath(val);
			if ("LOGFILE" == key) OpenDebugLog(getRelativePath()+val);
			mapArgs[key]=val;
		}
	}
}

void FileCommit(FILE *file)
{
    fflush(file); // harmless if redundantly called
#ifdef WIN32
    HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(file));
    FlushFileBuffers(hFile);
#else
    #if defined(__linux__) || defined(__NetBSD__)
    fdatasync(fileno(file));
    #elif defined(__APPLE__) && defined(F_FULLFSYNC)
    fcntl(fileno(file), F_FULLFSYNC, 0);
    #endif
#endif
}

//Note that the following functions trim by ref, modifying the original string without copying
inline void ltrim(std::string &s, const char* t) {
	s.erase(0, s.find_first_not_of(t));
}

inline void rtrim(std::string &s, const char* t) {
	s.erase(s.find_last_not_of(t) +1);
}

inline std::string trim(std::string &s) {
	ltrim(s, " \t\n\r\f\v");
	rtrim(s, " \t\n\r\f\v");
	return(s);
}

/**
 * this function tries to raise the file descriptor limit to the requested number.
 * It returns the actual file descriptor limit (which may be more or less than nMinFD)
 */
int RaiseFileDescriptorLimit(int nMinFD) {
#if defined(WIN32)
    return 2048;
#else
    struct rlimit limitFD;
    if (getrlimit(RLIMIT_NOFILE, &limitFD) != -1) {
        if (limitFD.rlim_cur < (rlim_t)nMinFD) {
            limitFD.rlim_cur = nMinFD;
            if (limitFD.rlim_cur > limitFD.rlim_max)
                limitFD.rlim_cur = limitFD.rlim_max;
            setrlimit(RLIMIT_NOFILE, &limitFD);
            getrlimit(RLIMIT_NOFILE, &limitFD);
        }
        return limitFD.rlim_cur;
    }
    return nMinFD; // getrlimit failed, assume it's fine
#endif
}

/**
 * this function tries to make a particular range of a file allocated (corresponding to disk space)
 * it is advisory, and the range specified in the arguments will never contain live data
 */
void AllocateFileRange(FILE *file, unsigned int offset, unsigned int length) {
#if defined(WIN32)
    // Windows-specific version
    HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(file));
    LARGE_INTEGER nFileSize;
    int64_t nEndPos = (int64_t)offset + length;
    nFileSize.u.LowPart = nEndPos & 0xFFFFFFFF;
    nFileSize.u.HighPart = nEndPos >> 32;
    SetFilePointerEx(hFile, nFileSize, 0, FILE_BEGIN);
    SetEndOfFile(hFile);
#elif defined(MAC_OSX)
    // OSX specific version
    fstore_t fst;
    fst.fst_flags = F_ALLOCATECONTIG;
    fst.fst_posmode = F_PEOFPOSMODE;
    fst.fst_offset = 0;
    fst.fst_length = (off_t)offset + length;
    fst.fst_bytesalloc = 0;
    if (fcntl(fileno(file), F_PREALLOCATE, &fst) == -1) {
        fst.fst_flags = F_ALLOCATEALL;
        fcntl(fileno(file), F_PREALLOCATE, &fst);
    }
    ftruncate(fileno(file), fst.fst_length);
#elif defined(__linux__)
    // Version using posix_fallocate
    off_t nEndPos = (off_t)offset + length;
    posix_fallocate(fileno(file), 0, nEndPos);
#else
    // Fallback version
    // TODO: just write one byte per block
    static const char buf[65536] = {};
    fseek(file, offset, SEEK_SET);
    while (length > 0) {
        unsigned int now = 65536;
        if (length < now)
            now = length;
        fwrite(buf, 1, now, file); // allowed to fail; this function is advisory anyway
        length -= now;
    }
#endif
}

void runCommand(const std::string& strCommand)
{
    if (strCommand.empty()) return;
    int nErr = ::system(strCommand.c_str());
    if (nErr)
        LogPrintf("runCommand error: system(%s) returned %d\n", strCommand, nErr);
}

void signalHandler(int signum) {
	StartShutdown();
	LogPrintStr("Signal ("+std::to_string(signum)+") received.");
	Shutdown();
}

void RenameThread(const char* name)
{
#if defined(PR_SET_NAME)
    // Only the first 15 characters are used (16 - NUL terminator)
    ::prctl(PR_SET_NAME, name, 0, 0, 0);
#elif (defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__))
    pthread_set_name_np(pthread_self(), name);

#elif defined(MAC_OSX)
    pthread_setname_np(name);
#else
    // Prevent warnings for unused parameters...
    (void)name;
#endif
}

void SetupEnvironment()
{
#ifdef HAVE_MALLOPT_ARENA_MAX
    // glibc-specific: On 32-bit systems set the number of arenas to 1.
    // By default, since glibc 2.10, the C library will create up to two heap
    // arenas per core. This is known to cause excessive virtual address space
    // usage. Work around it by setting the maximum number of arenas to 1.
    if (sizeof(void*) == 4) {
        mallopt(M_ARENA_MAX, 1);
    }
#endif
}

bool SetupNetworking()
{
#ifdef WIN32
    // Initialize Windows Sockets
    WSADATA wsadata;
    int ret = WSAStartup(MAKEWORD(2,2), &wsadata);
    if (ret != NO_ERROR || LOBYTE(wsadata.wVersion ) != 2 || HIBYTE(wsadata.wVersion) != 2)
        return false;
#endif
    return true;
}

// Obtain the application startup time (used for uptime calculation)
int64_t GetStartupTime()
{
    return nStartupTime;
}


