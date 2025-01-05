#pragma once

#include <exception>
#include <stdexcept>

namespace bml {

  /**
   * Exception type indicating an end of stream.
   *
   * - For inputs, this indicates no more data available to read from the input source.
   * - For outputs, this indicates no more space to write to the output sink.
   */
  struct EndOfStreamError final : public std::length_error {
    EndOfStreamError(const std::string &msg) : std::length_error(msg) {}
    EndOfStreamError(const char *msg) : std::length_error(msg) {}
  };
} // namespace bml
