/*
 * message_service.h - Communication constants
 *
 *  Created on: Mar 3, 2018
 *  Created by: Shawn McKenney
 */
#pragma once

#include <chrono>
#include <string>

namespace Devv {
namespace io {

namespace constants {
  // the default I/O read timeout in milliseconds
  static constexpr std::chrono::milliseconds kReadTimeout{500};
}

} // namespace io
} // namespace Devv
