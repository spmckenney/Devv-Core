#ifndef DEVCASH_LOGGER_H
#define DEVCASH_LOGGER_H

#include <thread>

#define BOOST_LOG_DYN_LINK \
  1  // necessary when linking the boost_log library dynamically

#include <boost/shared_ptr.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <cstdlib>

#include "common/minitrace.h"

// the logs are also written to LOGFILE
#define LOGFILE "/tmp/devcash.log"

// just log messages with severity >= SEVERITY_THRESHOLD are written
#define SEVERITY_THRESHOLD logging::trivial::warning

//Narrow-char thread-safe logger.
typedef boost::log::sources::severity_logger_mt<boost::log::trivial::severity_level> logger_t;

// register a global logger
BOOST_LOG_GLOBAL_LOGGER(logger, logger_t)

static boost::log::trivial::severity_level GetLogLevel() {
  const char* env_p = std::getenv("DEVCASH_OUTPUT_LOGLEVEL");

  if ( env_p == nullptr) {
    return boost::log::trivial::info;
  }

  std::string env_ps(env_p);
  std::transform(env_ps.begin(), env_ps.end(), env_ps.begin(), ::tolower);

  if (env_ps.find("trace") != std::string::npos) {
    return boost::log::trivial::trace;
  } else if (env_ps.find("debug") != std::string::npos) {
    return boost::log::trivial::debug;
  } else if (env_ps.find("info") != std::string::npos) {
    return boost::log::trivial::info;
  } else if (env_ps.find("warn") != std::string::npos) {
    return boost::log::trivial::warning;
  } else if (env_ps.find("error") != std::string::npos) {
    return boost::log::trivial::error;
  } else if (env_ps.find("fatal") != std::string::npos) {
    return boost::log::trivial::fatal;
  } else {
    return boost::log::trivial::info;
  }
}

static void init_log(void) {
  /* init boost log
   * 1. Add common attributes
   * 2. set log filter to trace
   */
  boost::log::add_common_attributes();
  boost::log::core::get()->set_filter(boost::log::trivial::severity >= GetLogLevel());
}

static inline std::string file_cut(const char* file) {
  std::string s(file);
  auto p = s.rfind("src");
  auto st = s.substr(p);
  return st;
}

// just a helper macro used by the macros below - don't use it in your code
#define LOG(severity) \
  BOOST_LOG_SEV(logger::get(), boost::log::trivial::severity) \
  << file_cut(__FILE__) << ":" << __LINE__ << " " << __FUNCTION__ << " -> "

// ===== log macros =====
#define LOG_TRACE LOG(trace)
#define LOG_DEBUG LOG(debug)
#define LOG_INFO LOG(info)
#define LOG_WARNING LOG(warning)
#define LOG_ERROR LOG(error)
#define LOG_FATAL LOG(fatal)

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

#define NOW std::chrono::high_resolution_clock::now()
#define MILLI_SINCE(start) std::chrono::duration_cast<std::chrono::milliseconds>(NOW - start).count()

#endif  // DEVCASH_LOGGER_H
