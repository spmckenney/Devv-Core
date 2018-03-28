/*
 * util.h general utilities.
 *
 *  Created on: Dec 27, 2017
 *  Author: Nick Williams
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
#include <bitset>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace Devcash
{

static std::string jsonFinder(std::string haystack, std::string key, size_t& pos) {
  std::string out;
  size_t dex = haystack.find("\""+key+"\":", pos);
  if (dex == std::string::npos) return out;
  dex += key.size()+3;
  size_t eDex = haystack.find(",\"", dex);
  if (eDex == std::string::npos) {
    eDex = haystack.find("}", dex);
    if (eDex == std::string::npos) return out;
  }
  out = haystack.substr(dex, eDex-dex);
  pos = eDex;
  if (out.front() == '\"') out.erase(std::begin(out));
  if (out.back() == '\"') out.pop_back();
  return out;
}

static std::vector<uint8_t> str2Bin(std::string msg) {
  std::vector<uint8_t> out;
  for (std::size_t i=0; i <msg.size(); ++i) {
    out.push_back((int) msg.at(i));
  }
  return out;
}

static std::string bin2Str(std::vector<uint8_t> bytes) {
  std::string out;
  for (std::size_t i=0; i <bytes.size(); ++i) {
    out += char(bytes[i]);
  }
  return out;
}

/** Setup the runtime environment. */
void SetupEnvironment();

/** Setup networking.
 *  @return true iff networking setup successfully
 *  @return false if an error may have occurred
*/
bool SetupNetworking();

/** Formats an exception for logging.
 *  @param pex pointer to the exception
 *  @param pszThread name of the thread/module where exception occurred
 *  @return a formatted string to log the exception
*/
std::string FormatException(const std::exception* pex,
  const std::string pszThread);

/** Sets the path for config files, inputs, and outputs
 *  @param path string for the working path
*/
void setWorkPath(std::string path);

/** Gets the path for config files, inputs, and outputs
 *  @return path string for the working path
*/
std::string getWorkPath();

/** Syncs a file
 *  @param file pointer to the FILE
*/
void FileCommit(FILE* file);

/** Left trims chars from a string.
 *  @note the string is modified by reference
 *  @param s the string to trim
 *  @param t pointer to chars that should be trimmed
*/
inline void ltrim(std::string& s, const char* t);

/** Right trims chars from a string.
 *  @note the string is modified by reference
 *  @param s the string to trim
 *  @param t pointer to chars that should be trimmed
*/
inline void rtrim(std::string& s, const char* t);

/** Trims whitespace chars from both sides of a string.
 *  @note the string is modified by reference
 *  @param s the string to trim
*/
inline std::string trim(std::string& s);

/** Attempts to raise the file descriptor limit.
 * This function tries to raise the file descriptor limit to the requested number.
 * It returns the actual file descriptor limit (more or less than nMinFD)
 *  @param nMinFD the target file descriptor limit
 *  @return the actual file descriptor limit
*/
int RaiseFileDescriptorLimit(int nMinFD);

/**
 * Tries to allocate disk space for a file
 * This function is advisory, not a guarantee.
 * The range specified in the arguments will never contain live data.
 * @param file pointer to the file
 * @param offset an allocation offset
 * @param length length of the requested allocation
 */
void AllocateFileRange(FILE* file, unsigned int offset, unsigned int length);

/**
 * Tries to write a string to a stream
 * @param str the string to write
 * @param fout pointer to the stream to write
 * @return number of bytes written
 */
int StreamWriteStr(const std::string& str, std::ostream* fout);

/**
 * Reads a file into a string
 * @param filePath the path of the file
 * @return if the file is readable, returns a string of its contents
 * @return if file is not readable, returns an empty string
 */
std::string ReadFile(const std::string& filePath);

/**
 * Reacts to an OS signal.
 * Currently shuts down on any signal.
 * @param signum the signal
 */
void signalHandler(int signum);

/**
 * Checks for switch char
 * @param c the character to check
 * @return true iff char is a switch in this environment
 * @return false otherwise
 */
inline bool IsSwitchChar(char c)
{
#ifdef WIN32
    return c == '-' || c == '/';
#else
    return c == '-';
#endif
}

/** Maps a hex digit to an int value.
 *  @param hex digit to get the int value for
 *  @return int value of this hex digit
*/
static int char2int(char in) {
  if (in >= '0' && in <= '9') {
    return(in - '0');
  }
  if (in >= 'A' && in <= 'F') {
    return(in - 'A' + 10);
  }
  if (in >= 'a' && in <= 'f') {
  return(in - 'a' + 10);
  }
  return(-1);
}

/** Maps a hex string into a byte vector for CBOR parsing.
 *  @param hex a string of hex digits encoding a CBOR message.
 *  @return a byte vector of the same data in binary form.
*/
static std::vector<uint8_t> hex2CBOR(std::string hex) {
  int len = hex.length();
  std::vector<uint8_t> buf(len/2+1);
  for (int i=0;i<len/2;i++) {
    buf.at(i) = (uint8_t) char2int(hex.at(i*2))*16+char2int(hex.at(1+i*2));
  }
  buf.pop_back(); //remove null terminator
  return(buf);
}

/** Maps a CBOR byte vector to a hex string.
 *  @param b a CBOR byte vector to encode as hex digits.
 *  @return a string of hex digits encoding this byte vector.
*/
/*static std::string CBOR2hex(std::vector<uint8_t> b) {
  int len = b.size();
  std::stringstream ss;
  for (int j=0; j<len; j++) {
  int c = (int) b[j];
  ss.put(alpha[(c>>4)&0xF]);
  ss.put(alpha[c&0xF]);
  }
  return(ss.str());
}*/

class ArgsManager
{
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

protected:
    std::map<std::string, std::string> mapArgs;
    std::map<std::string, std::vector<std::string>> mapMultiArgs;
};

extern ArgsManager gArgs;

} //end namespace Devcash

#endif // DEVCASH_UTIL_H

