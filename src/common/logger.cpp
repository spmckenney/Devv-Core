#include "common/logger.h"

#include <boost/log/core/core.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <fstream>
#include <ostream>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;

BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity",
                            logging::trivial::severity_level)

BOOST_LOG_GLOBAL_LOGGER_INIT(logger, src::severity_logger_mt) {
  //src::severity_logger_mt logger;
  logger_t logger;

  boost::shared_ptr< logging::core > core = logging::core::get();

  // Create a backend and attach a couple of streams to it
  boost::shared_ptr< sinks::text_ostream_backend > backend =
    boost::make_shared< sinks::text_ostream_backend >();
  backend->add_stream(
                      boost::shared_ptr< std::ostream >(&std::clog, boost::null_deleter()));
  backend->add_stream(
                      boost::shared_ptr< std::ostream >(new std::ofstream("/tmp/devcash.log")));
  
  // Enable auto-flushing after each log record written
  backend->auto_flush(true);

  // Wrap it into the frontend and register in the core.
  // The backend requires synchronization in the frontend.
  typedef sinks::synchronous_sink< sinks::text_ostream_backend > sink_t;
  boost::shared_ptr< sink_t > sink(new sink_t(backend));
  core->add_sink(sink);
  
  return logger;
}
