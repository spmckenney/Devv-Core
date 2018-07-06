/*
 * devcash_exceptions.h
 *
 *  Created on: Jun 19, 2018
 *      Author: Shawn McKenney
 */
#pragma once

#include <stdexcept>
#include <exception>

namespace Devcash {

/**
 * An error occured while handling an incoming DevcashMessage
 * Usually these errors occur due to malformed input and are
 * recoverable.
 */
struct DevcashMessageError : public std::runtime_error {

  /// Default constructor
  DevcashMessageError() = default;

  /**
   * Constructor
   * @param message human-readable error message
   * @return
   */
  explicit DevcashMessageError(const std::string& message)
      : std::runtime_error(message)
  {}

  /// Default destructor
  ~DevcashMessageError() override = default;
};

/**
 * Indicates an error deserializing an incoming message
 */
struct DeserializationError : public DevcashMessageError {
  /**
   * Constructor
   * @param message human-readable error message
   */
  explicit DeserializationError(const std::string& message)
      : DevcashMessageError(message)
  {}

  /**
   * Destructor.
   */
  ~DeserializationError() override = default;
};

} // namespace Devcash
