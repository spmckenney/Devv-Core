/*
 * util.cpp general utilities.
 *
 *  Created on: Dec 27, 2017
 *  Author: Nick Williams
 */

#include "util.h"

#include <iostream>
#include <fstream>
#include <stdarg.h>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <locale>
#include <unistd.h>
#include <chrono>

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
#endif

#include <openssl/crypto.h>
#include <openssl/rand.h>
#include <openssl/conf.h>

#include "../init.h"
#include "logger.h"

namespace Devcash
{

std::string workPath(""); /** Path to work directory. */

int StreamWriteStr(const std::string &str, std::ostream* fout)
{
  *fout << str << std::endl;
  return sizeof(str);
}

std::string ReadFile(const std::string& filePath)
{
  std::ifstream file(filePath);
  if (!file.good()) LOG_WARNING << "File "+filePath+" could not be found";
  if (!file.is_open())
    LOG_WARNING << "File "+filePath+" could not be opened, check permissions";

  std::string output("");
  for(std::string line; std::getline(file, line);) {
    output += line;
  }
  return(output);
}

/** Interpret string as boolean, for argument parsing */
static bool InterpretBool(const std::string& strValue)
{
  if (strValue.empty()) return true;
  return (std::stoi(strValue) != 0);
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

    if (str[0] != '-') break;

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
  auto it = mapMultiArgs.find(strArg);
  if (it != mapMultiArgs.end()) return it->second;
  return {};
}

bool ArgsManager::IsArgSet(const std::string& strArg) const
{
  return mapArgs.count(strArg);
}

std::string ArgsManager::GetArg(const std::string& strArg, const std::string& strDefault) const
{
  auto it = mapArgs.find(strArg);
  if (it != mapArgs.end()) return it->second;
  return strDefault;
}

std::string ArgsManager::GetPathArg(const std::string& strArg) const
{
  std::string arg(GetArg(strArg, ""));
  return(getWorkPath()+arg);
}

bool ArgsManager::GetBoolArg(const std::string& strArg, bool fDefault) const
{
  auto it = mapArgs.find(strArg);
  if (it != mapArgs.end()) return InterpretBool(it->second);
  return fDefault;
}

bool ArgsManager::SoftSetArg(const std::string& strArg, const std::string& strValue)
{
  if (IsArgSet(strArg)) return false;
  ForceSetArg(strArg, strValue);
  return true;
}

bool ArgsManager::SoftSetBoolArg(const std::string& strArg, bool fValue)
{
  if (fValue) return SoftSetArg(strArg, std::string("1"));
  else return SoftSetArg(strArg, std::string("0"));
}

void ArgsManager::ForceSetArg(const std::string& strArg, const std::string& strValue)
{
  mapArgs[strArg] = strValue;
  mapMultiArgs[strArg] = {strValue};
}

std::string FormatException(const std::exception* pex, const std::string pszThread)
{
  std::string msg = "";
  if (pex) {
    msg += "EXCEPTION[";
    msg += pszThread;
    msg += "]";
    msg += ": ";
    msg += std::string(pex->what());
  } else {
    msg += "UNKNOWN EXCEPTION[";
    msg += pszThread;
    msg += "]";
  }
  msg += "\n";
  return(msg);
}

void setWorkPath(std::string arg0) {
  workPath = trim(arg0);
}
std::string getWorkPath() {
  return(workPath);
}

void ArgsManager::ReadConfigFile(const std::string& confPath)
{
  std::ifstream streamConfig(confPath);
  if (!streamConfig.good())
    throw std::runtime_error("Config file could not be found");
  if (!streamConfig.is_open())
    throw std::runtime_error("Couldn't open config file, check permissions");

  for(std::string line; std::getline(streamConfig, line);) {
    std::string::size_type eq = line.find("=");
    if (eq != std::string::npos) {
      std::string key = line.substr(0, eq);
      std::string val = line.substr(eq+1);
      if ("RELPATH" == key) setWorkPath(val);
      if ("LOGFILE" == key) {
        std::cout << "Check DevCash logs at "+key << std::endl;
        LOG_INFO << "DevCash initializing...";
      }
      mapArgs[key]=val;
    } //endif line has "="
  } //end for each line
}

void FileCommit(FILE* file)
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

//Note that the following functions trim by ref
//modifying the original string without copying
inline void ltrim(std::string& s, const char* t) {
  s.erase(0, s.find_first_not_of(t));
}

inline void rtrim(std::string& s, const char* t) {
  s.erase(s.find_last_not_of(t) +1);
}

inline std::string trim(std::string& s) {
  ltrim(s, " \t\n\r\f\v");
  rtrim(s, " \t\n\r\f\v");
  return(s);
}

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

void AllocateFileRange(FILE* file, unsigned int offset, unsigned int length) {
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
    //fwrite allowed to fail; this function is advisory anyway
    fwrite(buf, 1, now, file);
    length -= now;
  }
#endif
}

void signalHandler(int signum) {
  StartShutdown();
  LOG_INFO << "Signal ("+std::to_string(signum)+") received.";
  Shutdown();
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

} //end namespace Devcash
