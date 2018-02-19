/*
 * util.h
 *
 *  Created on: Dec 27, 2017
 *      Author: Nick Williams
 */

/**
 * Server/client environment: argument handling, config file parsing,
 * logging, thread wrappers, startup time
 */
#ifndef DEVCASH_UTIL_H
#define DEVCASH_UTIL_H

#include <atomic>
#include <exception>
#include <map>
#include <stdint.h>
#include <string>
#include <vector>

static const bool DEFAULT_LOGTIMEMICROS = false;
static const bool DEFAULT_LOGIPS        = false;
static const bool DEFAULT_LOGTIMESTAMPS = true;
extern const char * const DEFAULT_DEBUGLOGFILE;

extern bool fPrintToConsole;
extern bool fPrintToDebugLog;

extern bool fLogTimestamps;
extern bool fLogTimeMicros;
extern bool fLogIPs;
extern std::atomic<bool> fReopenDebugLog;

extern const char * const DEVCASH_CONF_FILENAME;
extern const char * const DEVCASH_PID_FILENAME;

extern std::atomic<uint32_t> logCategories;

void SetupEnvironment();
bool SetupNetworking();

struct CLogCategoryActive
{
    std::string category;
    bool active;
};

namespace DCLog {
    enum LogFlags : uint32_t {
        NONE        = 0,
        NET         = (1 <<  0),
		INIT		= (1 <<  1),
        MEMPOOL     = (1 <<  2),
        HTTP        = (1 <<  3),
        BENCH       = (1 <<  4),
        DB          = (1 <<  5),
        ESTIMATEFEE = (1 <<  6),
        ADDRMAN     = (1 <<  7),
        SELECTCOINS = (1 <<  8),
        REINDEX     = (1 <<  9),
        CMPCTBLOCK  = (1 << 10),
        CRYPTO      = (1 << 11),
        PRUNE       = (1 << 12),
        PROXY       = (1 << 13),
        MEMPOOLREJ  = (1 << 14),
        LIBEVENT    = (1 << 15),
        COINDB      = (1 << 16),
        LEVELDB     = (1 << 17),
        ALL         = ~(uint32_t)0,
    };
}
/** Return true if log accepts specified category */
static inline bool LogAcceptCategory(uint32_t category)
{
    return (logCategories.load(std::memory_order_relaxed) & category) != 0;
}

/** Returns a string with the log categories. */
std::string ListLogCategories();

/** Returns a vector of the active log categories. */
std::vector<CLogCategoryActive> ListActiveLogCategories();

/** Return true if str parses as a log category and set the flags in f */
bool GetLogCategory(uint32_t *f, const std::string *str);

/** Send a string to the log output */
int LogPrintStr(const std::string &str);

/** Get format string from VA_ARGS for error reporting */
template<typename... Args> std::string FormatStringFromLogArgs(const char *fmt, const Args&... args) { return fmt; }

static inline void MarkUsed() {}
template<typename T, typename... Args> static inline void MarkUsed(const T& t, const Args&... args)
{
    (void)t;
    MarkUsed(args...);
}

std::string FormatException(const std::exception* pex, const std::string pszThread, uint32_t catNum);
void setRelativePath(std::string path);
std::string getRelativePath();
void FileCommit(FILE *file);
inline void ltrim(std::string &s, const char* t);
inline void rtrim(std::string &s, const char* t);
inline std::string trim(std::string &s);
int RaiseFileDescriptorLimit(int nMinFD);
void AllocateFileRange(FILE *file, unsigned int offset, unsigned int length);
int FileWriteStr(const std::string &str, std::ostream* fout);
std::string ReadFile(const std::string& filePath);
bool OpenDebugLog(std::string logPath);
void runCommand(const std::string& strCommand);
void signalHandler(int signum);

inline bool IsSwitchChar(char c)
{
#ifdef WIN32
    return c == '-' || c == '/';
#else
    return c == '-';
#endif
}

class ArgsManager
{
protected:
    std::map<std::string, std::string> mapArgs;
    std::map<std::string, std::vector<std::string>> mapMultiArgs;
public:
    void ParseParameters(int argc, const char*const argv[]);
    void ReadConfigFile(const std::string& confPath);

    /**
     * Return a vector of strings of the given argument
     *
     * @param strArg Argument to get (e.g. "-foo")
     * @return command-line arguments
     */
    std::vector<std::string> GetArgs(const std::string& strArg) const;

    /**
     * Return true if the given argument has been manually set
     *
     * @param strArg Argument to get (e.g. "-foo")
     * @return true if the argument has been set
     */
    bool IsArgSet(const std::string& strArg) const;

    /**
     * Return string argument or default value
     *
     * @param strArg Argument to get (e.g. "-foo")
     * @param strDefault (e.g. "1")
     * @return command-line argument or default value
     */
    std::string GetArg(const std::string& strArg, const std::string& strDefault) const;

    /**
     * Return a file path argument or an empty string.
     * Adds the relative path for this program if no path was specified in the map
     *
     * @param strArg Argument to get (e.g. "LOGFILE")
     * @return file path config argument or an empty string
     */
    std::string GetPathArg(const std::string& strArg) const;

    /**
     * Return boolean argument or default value
     *
     * @param strArg Argument to get (e.g. "-foo")
     * @param fDefault (true or false)
     * @return command-line argument or default value
     */
    bool GetBoolArg(const std::string& strArg, bool fDefault) const;

    /**
     * Set an argument if it doesn't already have a value
     *
     * @param strArg Argument to set (e.g. "-foo")
     * @param strValue Value (e.g. "1")
     * @return true if argument gets set, false if it already had a value
     */
    bool SoftSetArg(const std::string& strArg, const std::string& strValue);

    /**
     * Set a boolean argument if it doesn't already have a value
     *
     * @param strArg Argument to set (e.g. "-foo")
     * @param fValue Value (e.g. false)
     * @return true if argument gets set, false if it already had a value
     */
    bool SoftSetBoolArg(const std::string& strArg, bool fValue);

    // Forces an arg setting. Called by SoftSetArg() if the arg hasn't already
    // been set. Also called directly in testing.
    void ForceSetArg(const std::string& strArg, const std::string& strValue);
};

extern ArgsManager gArgs;

/**
 * Format a string to be used as group of options in help messages
 *
 * @param message Group name (e.g. "RPC server options:")
 * @return the formatted string
 */
std::string HelpMessageGroup(const std::string& message);

/**
 * Format a string to be used as option description in help messages
 *
 * @param option Option message (e.g. "-rpcuser=<user>")
 * @param message Option description (e.g. "Username for JSON-RPC connections")
 * @return the formatted string
 */
std::string HelpMessageOpt(const std::string& option, const std::string& message);

#endif // DEVCASH_UTIL_H

