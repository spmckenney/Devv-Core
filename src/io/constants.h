/*
 * message_service.h - Communication constants
 *
 *  Created on: Mar 3, 2018
 *  Created by: Shawn McKenney
 */
#pragma once

#include <folly/Range.h>
#include <chrono>
#include <string>

namespace Devcash {
namespace io {

  namespace constants {
  // the zmq url for transaction messages
  static constexpr folly::StringPiece kTransactionMessageUrl = "tcp://127.0.0.1:55556";

  // the default I/O read timeout in milliseconds
  static constexpr std::chrono::milliseconds kReadTimeout{500};
  }
} // namespace io
} // namespace Devcash
