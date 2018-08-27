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

BOOST_LOG_GLOBAL_LOGGER_INIT(logger, src::severity_logger_mt) {
  logger_t logger;
  return logger;
}

/*
  boost::shared_ptr< logging::core > core = logging::core::get();

  bool add_ostream_sink = true;
  if (add_ostream_sink) {
    // Create a backend and attach a couple of streams to it
    boost::shared_ptr<sinks::text_ostream_backend> backend =
        boost::make_shared<sinks::text_ostream_backend>();

    typedef boost::shared_ptr<std::ostream> OStreamPtr;
    backend->add_stream(OStreamPtr(&std::clog, boost::null_deleter()));
    //backend->add_stream(OStreamPtr(new std::ofstream(LOGFILE)));

    // Enable auto-flushing after each log record written
    backend->auto_flush(true);

    // Wrap it into the frontend and register in the core.
    // The backend requires synchronization in the frontend.
    typedef sinks::synchronous_sink<sinks::text_ostream_backend> sink_t;
    boost::shared_ptr<sink_t> sink(new sink_t(backend));

    sink->set_formatter
        (
            expr::stream
                << std::hex << std::setw(6) << std::setfill('0') << line_id << std::dec << std::setfill(' ')
                << " " << timestamp
                << " " << severity
                << " " << std::hex << std::setw(6) << thread_id
                << " " << expr::smessage
        );

    core->add_sink(sink);
  }
  // Add syslog sink
  bool add_syslog_sink = true;
  if (add_syslog_sink) {
    // Create a new backend
    boost::shared_ptr< sinks::syslog_backend > backend(new sinks::syslog_backend(
        logging::keywords::facility = sinks::syslog::local0,
        logging::keywords::use_impl = sinks::syslog::udp_socket_based
    ));

    // Setup the target address and port to send syslog messages to
    backend->set_target_address("127.0.0.1", 514);

    // Wrap it into the frontend and register in the core.
    // The backend requires synchronization in the frontend.
    typedef sinks::synchronous_sink<sinks::syslog_backend> sink_t;
    boost::shared_ptr<sink_t> sink(new sink_t(backend));

    core->add_sink(sink);
  }
  return logger;
}
*/
