#pragma once

#include "reader.hpp"
#include "writer.hpp"

#include <algorithm>
#include <span>

namespace bml {

  // Helper class for std::visit, see https://en.cppreference.com/w/cpp/utility/variant/visit
  template <class... Ts>
  struct overloaded : Ts... {
    using Ts::operator()...;
  };

  template <class... Ts>
  overloaded(Ts &&...) -> overloaded<Ts...>;

  [[nodiscard]] inline bool extractNextByte(BitReader::ByteSource &source, std::byte &out) {
    return std::visit<bool>(overloaded{[&out](BitReader::ByteGenerator &generator) { return generator(out); },
                                       [&out](BitReader::ByteRange &range) {
                                         if (range.begin == range.end) {
                                           return false;
                                         }
                                         out = *range.begin++;
                                         return true;
                                       },
                                       [](std::monostate) { return false; }},
                            source);
  }

  inline bool extractNextBytes(BitReader::ByteSource &source, std::span<std::byte> outBytes) {
    return std::visit<bool>(overloaded{[outBytes](BitReader::ByteGenerator &generator) {
                                         for (auto &b : outBytes) {
                                           if (!generator(b)) {
                                             return false;
                                           }
                                         }
                                         return true;
                                       },
                                       [outBytes](BitReader::ByteRange &range) {
                                         auto byteCount = static_cast<std::ptrdiff_t>(outBytes.size());
                                         if (std::distance(range.begin, range.end) < byteCount) {
                                           return false;
                                         }
                                         std::copy_n(range.begin, outBytes.size(), outBytes.begin());
                                         range.begin += byteCount;
                                         return true;
                                       },
                                       [](std::monostate) { return false; }},
                            source);
  }

  [[nodiscard]] inline bool writeNextByte(BitWriter::ByteSink &sink, std::byte byte) {
    return std::visit<bool>(overloaded{[byte](BitWriter::ByteConsumer &consumer) { return consumer(byte); },
                                       [byte](BitWriter::ByteRange &range) {
                                         if (range.begin == range.end) {
                                           return false;
                                         }
                                         *range.begin++ = byte;
                                         return true;
                                       },
                                       [](std::monostate) { return false; }},
                            sink);
  }

  [[nodiscard]] inline bool writeNextBytes(BitWriter::ByteSink &sink, std::span<const std::byte> bytes) {
    return std::visit<bool>(overloaded{[bytes](BitWriter::ByteConsumer &consumer) {
                                         return std::all_of(bytes.begin(), bytes.end(), consumer);
                                       },
                                       [bytes](BitWriter::ByteRange &range) {
                                         auto byteCount = static_cast<std::ptrdiff_t>(bytes.size());
                                         if (std::distance(range.begin, range.end) < byteCount) {
                                           return false;
                                         }
                                         range.begin = std::copy(bytes.begin(), bytes.end(), range.begin);
                                         return true;
                                       },
                                       [](std::monostate) { return false; }},
                            sink);
  }

} // namespace bml
