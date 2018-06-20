/*
 * devcash_exceptions.h
 *
 *  Created on: Jun 19, 2018
 *      Author: Shawn McKenney
 */
#pragma once

#include <stdexcept>
#include <bits/exception.h>

namespace Devcash {

/**
 * Thrown to indication a deserialization error
 */
struct DeserializationError : public std::runtime_error {
  /**
   * Constructor (C++ STL strings).
   *  @param message The error message.
   */
  explicit DeserializationError(const std::string& message)
      : std::runtime_error(message)
  {}

  /**
   * Destructor.
   * Virtual to allow for subclassing.
   */
  ~DeserializationError() override = default;

  /**
   * Returns a pointer to the (constant) error description.
   *  @return A pointer to a const char*. The underlying memory
   *          is in possession of the Exception object. Callers must
   *          not attempt to free the memory.
   */
  const char* what() const noexcept override {
    return std::runtime_error::what();
  }
};

} // namespace Devcash