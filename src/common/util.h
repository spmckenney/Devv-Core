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
#include <chrono>
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

typedef unsigned char byte;

namespace Devcash
{

typedef boost::multiprecision::uint256_t uint256_t;

static int32_t BinToInt32(const std::vector<byte>& bytes, size_t start) {
  int32_t dest = 0;
  for (unsigned int i=0; i<4; ++i) {
    dest |= (bytes.at(start+i) << (i*8));
  }
  return dest;
}

static std::vector<byte> Int32ToBin(const int32_t& source
    ,std::vector<byte>& dest) {
  for (unsigned int i=0; i<4; ++i) {
    dest.push_back((source >> (i*8)) & 0xFF);
  }
  return dest;
}

static uint32_t BinToUint32(const std::vector<byte>& bytes, size_t start) {
  uint32_t dest = 0;
  for (unsigned int i=0; i<4; ++i) {
    dest |= (bytes.at(start+i) << (i*8));
  }
  return dest;
}

static std::vector<byte> Uint32ToBin(const uint32_t& source
    ,std::vector<byte>& dest) {
  for (unsigned int i=0; i<4; ++i) {
    dest.push_back((source >> (i*8)) & 0xFF);
  }
  return dest;
}

static uint64_t BinToUint64(const std::vector<byte>& bytes, size_t start) {
  uint32_t lsb = BinToUint32(bytes, start);
  uint32_t msb = BinToUint32(bytes, start+4);
  uint64_t dest = (uint64_t(msb) << 32) | uint64_t(lsb);
  return dest;
}

static std::vector<byte> Uint64ToBin(const uint64_t& source
    ,std::vector<byte>& dest) {
  uint32_t lsb = source&0xffffffff;
  uint32_t msb = source >> 32;
  Uint32ToBin(lsb, dest);
  Uint32ToBin(msb, dest);
  return dest;
}

static int64_t BinToInt64(const std::vector<byte>& bytes, size_t start) {
  int32_t lsb = BinToInt32(bytes, start);
  int32_t msb = BinToInt32(bytes, start+4);
  int64_t dest = (int64_t(msb) << 32) | int64_t(lsb);
  return dest;
}

static std::vector<byte> Int64ToBin(const int64_t& source
    ,std::vector<byte>& dest) {
  int32_t lsb = source&0xffffffff;
  int32_t msb = source >> 32;
  Int32ToBin(lsb, dest);
  Int32ToBin(msb, dest);
  return dest;
}

static uint64_t getEpoch() {
  std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>
    (std::chrono::system_clock::now().time_since_epoch());
  return ms.count();
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
static std::string toHex(const std::vector<byte>& input) {
  std::stringstream ss;
  for (size_t j=0; j<input.size(); j++) {
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
    return (end_.tv_sec - beg_.tv_sec)*1000 +
      (end_.tv_nsec - beg_.tv_nsec) / 1000000.;
  }

  void reset() { clock_gettime(CLOCK_REALTIME, &beg_); }

private:
  timespec beg_, end_;
};

} //end namespace Devcash

#endif // DEVCASH_UTIL_H
