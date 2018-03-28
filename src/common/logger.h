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
// the logs are also written to LOGFILE
#define LOGFILE "logfile.log"

// just log messages with severity >= SEVERITY_THRESHOLD are written
#define SEVERITY_THRESHOLD logging::trivial::warning

//Narrow-char thread-safe logger.
typedef boost::log::sources::severity_logger_mt<boost::log::trivial::severity_level> logger_t;

// register a global logger
BOOST_LOG_GLOBAL_LOGGER(logger, logger_t)

static void init_log(void)
{
  /* init boost log
   * 1. Add common attributes
   * 2. set log filter to trace
   */
  boost::log::add_common_attributes();
  boost::log::core::get()->add_global_attribute("Scope",
                                                boost::log::attributes::named_scope());
  boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::trace);
}

// just a helper macro used by the macros below - don't use it in your code
#define LOG(severity) \
  BOOST_LOG_SEV(logger::get(), boost::log::trivial::severity) \
  << __FILENAME__ << ":" << __LINE__ << " TID[" << std::this_thread::get_id() << "] -> "

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

static const int kDEFAULT_WORKERS = 10;

#define NOW std::chrono::high_resolution_clock::now()
#define MILLI_SINCE(start) std::chrono::duration_cast<std::chrono::milliseconds>(NOW - start).count()

#endif  // DEVCASH_LOGGER_H
