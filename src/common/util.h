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
#include <memory>
#include <boost/multiprecision/cpp_int.hpp>

namespace Devcash
{

typedef unsigned char byte;
typedef boost::multiprecision::uint256_t uint256_t;

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

static std::vector<byte> Str2Bin(std::string msg) {
  std::vector<uint8_t> out;
  for (std::size_t i=0; i <msg.size(); ++i) {
    out.push_back((int) msg.at(i));
  }
  return out;
}

static std::string Bin2Str(std::vector<byte> bytes) {
  std::string out;
  for (std::size_t i=0; i <bytes.size(); ++i) {
    out += char(bytes[i]);
  }
  return out;
}

static uint256_t BinToUint256(const std::vector<byte>& bytes, unsigned int start
    , uint256_t& dest) {
  for (unsigned int i=0; i<32; ++i) {
    dest |= (bytes.at(start+i) << (i*8));
  }
  return dest;
}

static std::vector<byte> Uint256ToBin(const uint256_t& source
    , std::vector<byte>& dest) {
  for (unsigned int i=0; i<32; ++i) {
    dest.push_back((byte) (source >> (i*8)) & 0xFF);
  }
  return dest;
}

static uint64_t BinToUint64(const std::vector<byte>& bytes, unsigned int start
    , uint64_t& dest) {
  for (unsigned int i=0; i<8; ++i) {
    dest |= (bytes.at(start+i) << (i*8));
  }
  return dest;
}

static std::vector<byte> Uint64ToBin(const uint64_t& source
    , std::vector<byte>& dest) {
  for (unsigned int i=0; i<8; ++i) {
    dest.push_back((source >> (i*8)) & 0xFF);
  }
  return dest;
}

/** Maps a hex digit to an int value.
 *  @param hex digit to get the int value for
 *  @return int value of this hex digit
*/
static int Char2Int(char in) {
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
static std::vector<byte> Hex2Bin(std::string hex) {
  int len = hex.length();
  std::vector<uint8_t> buf(len/2+1);
  for (int i=0;i<len/2;i++) {
    buf.at(i) = (byte) Char2Int(hex.at(i*2))*16+Char2Int(hex.at(1+i*2));
  }
  buf.pop_back(); //remove null terminator
  return(buf);
}

/** chars for hex conversions */
static const char alpha[] = "0123456789ABCDEF";
/** Change binary data into a hex string.
 *  @pre input must be allocated to a length of len
 *  @param input pointer to the binary data
 *  @param len length of the binary data
 *  @return string containing these data as hex numbers
 */
static std::string toHex(const byte* input, size_t len) {
  std::stringstream ss;
  for (int j=0; j<len; j++) {
    int c = (int) input[j];
    ss.put(alpha[(c>>4)&0xF]);
    ss.put(alpha[c&0xF]);
  }
  return(ss.str());
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

template<typename T, typename ...Args>
std::unique_ptr<T> make_unique( Args&& ...args )
{
    return std::unique_ptr<T>( new T( std::forward<Args>(args)... ) );
}

/**
 * Time. A simple timer class.
 */
class Timer
{
public:
  Timer() { clock_gettime(CLOCK_REALTIME, &beg_); }

  double operator()() { return elapsed(); }

  double elapsed() {
    clock_gettime(CLOCK_REALTIME, &end_);
    return end_.tv_sec - beg_.tv_sec +
      (end_.tv_nsec - beg_.tv_nsec) / 1000000.;
  }

  void reset() { clock_gettime(CLOCK_REALTIME, &beg_); }

private:
  timespec beg_, end_;
};

} //end namespace Devcash

#endif // DEVCASH_UTIL_H
