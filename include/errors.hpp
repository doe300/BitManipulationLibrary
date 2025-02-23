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

  /**
   * Exception type indicating a calculated checksum or other error detection code does not match its expected value.
   */
  struct ChecksumMismatchError final : public std::runtime_error {
    ChecksumMismatchError(const std::string &msg) : std::runtime_error(msg) {}
    ChecksumMismatchError(const char *msg) : std::runtime_error(msg) {}
  };
} // namespace bml
