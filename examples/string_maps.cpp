/**
 * This example code shows how binary maps can be used to read the same (foreign-code) type in different ways by using
 * different mapping functions.
 */
#include "binary_map.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <span>
#include <string>
#include <vector>

using namespace bml::literals;

static const std::string EXPECTED_VALUE = "Foo Bar";

/**
 * Maps a std::string from/to binary prefix-string representation.
 *
 * The binary representation has following layout:
 *
 * - 1 Byte: size N in characters
 * - N Bytes: string contents
 */
struct MapPrefixString {
  using type = std::string;

  std::string operator()(bml::BitReader &reader) const {
    auto numBytes = mapSize(reader);
    return MapString::read(reader, mapChar, numBytes);
  }

  void operator()(bml::BitWriter &writer, const std::string &value) const {
    mapSize(writer, value.size());
    MapString::write(writer, value, mapChar, value.size());
  }

  static constexpr auto mapSize = bml::mapBytes<std::size_t>(1_bytes);
  static constexpr auto mapChar = bml::mapBytes<char>();
  using MapString = bml::MapContainer<std::string, decltype(mapChar)>;
};

/**
 * Maps a std::string from/to binary fixed-size string representation.
 *
 * The binary representation has following layout:
 *
 * - N Bytes: string contents
 */
struct MapFixedSizeString {
  using type = std::string;

  std::string operator()(bml::BitReader &reader) const {
    return MapString::read(reader, mapChar, EXPECTED_VALUE.size());
  }

  void operator()(bml::BitWriter &writer, const std::string &value) const {
    MapString::write(writer, value, mapChar, EXPECTED_VALUE.size());
  }

  static constexpr auto mapChar = bml::mapBytes<char>();
  using MapString = bml::MapContainer<std::string, decltype(mapChar)>;
};

/**
 * Maps a std::string from/to binary zero-terminated string representation.
 *
 * The binary representation has following layout:
 *
 * - ? Bytes: string contents
 * - 1 Byte: NUL-Byte terminating the string
 */
struct MapZeroTerminatedString {
  using type = std::string;

  std::string operator()(bml::BitReader &reader) const {
    std::string value{};
    while (auto c = mapChar(reader)) {
      value.push_back(c);
    }
    return value;
  }

  void operator()(bml::BitWriter &writer, const std::string &value) const {
    for (auto c : value) {
      mapChar(writer, c);
      if (c == '\0') {
        break;
      }
    }
  }

  static constexpr auto mapChar = bml::mapBytes<char>();
};

/**
 * Maps the string according to some arbitrary compression.
 *
 * Only alphabetical characters are stored in 6 bits, starting with 'a' stored as 0, 'z' as 25, 'A' as 32 and 'Z' as 57
 * with the special 6-bit values of 62 for a space (' ') of 63 indicating "end of string".
 */
struct MapCompressedString {
  using type = std::string;

  std::string operator()(bml::BitReader &reader) const {
    std::string value{};
    uint8_t code;
    while ((code = mapCode(reader)) < 63U) {
      if (code == 62U) {
        value.push_back(' ');
      } else if (code < 32U) {
        value.push_back(static_cast<char>('a' + code));
      } else {
        value.push_back(static_cast<char>('A' + (code - 32U)));
      }
    }
    return value;
  }

  void operator()(bml::BitWriter &writer, const std::string &value) const {
    for (auto c : value) {
      if (c >= 'a' && c <= 'z') {
        mapCode(writer, static_cast<uint8_t>(c - 'a'));
      } else if (c >= 'A' && c <= 'Z') {
        mapCode(writer, static_cast<uint8_t>((c - 'A') + 32));
      } else if (c == ' ') {
        mapCode(writer, 62U);
      }
    }
    mapCode(writer, 63U);
  }

  static constexpr auto mapCode = bml::mapBits<uint8_t>(6_bits);
};

template <typename T, typename C>
static bool checkMapper(const C &container, std::string_view name) {
  static_assert(bml::concepts::DirectMapper<T>);

  T mapper{};
  bml::BitReader reader{std::as_bytes(std::span{container})};
  auto string = mapper(reader);

  std::vector<std::byte> outBuffer(32);
  bml::BitWriter writer{std::span{outBuffer}};
  mapper(writer, string);

  if (EXPECTED_VALUE != string) {
    std::cerr << name << " does not match expected value: " << string << std::endl;
    return false;
  }

  if (!std::equal(container.begin(), container.end(), outBuffer.begin(),
                  [](char c, std::byte b) { return std::bit_cast<std::byte>(c) == b; })) {
    std::cerr << name << " does not match original buffer!" << std::endl;
    return false;
  }
  return true;
}

int main() {
  static const std::vector<char> PREFIX_STRING{7, 'F', 'o', 'o', ' ', 'B', 'a', 'r'};
  static const std::array<char, 7> FIXED_STRING{'F', 'o', 'o', ' ', 'B', 'a', 'r'};
  static const std::vector<char> ZERO_TERMINATED_STRING{'F', 'o', 'o', ' ', 'B', 'a', 'r', '\0'};
  static const std::vector<uint8_t> COMPRESSED_STRING{0x94, 0xE3, 0xBE, 0x84, 0x04, 0x7F};

  bool success = true;

  if (!checkMapper<MapPrefixString>(PREFIX_STRING, "Prefix string")) {
    success = false;
  }
  if (!checkMapper<MapFixedSizeString>(FIXED_STRING, "Fixed-size string")) {
    success = false;
  }
  if (!checkMapper<MapZeroTerminatedString>(ZERO_TERMINATED_STRING, "Zero-terminated string")) {
    success = false;
  }
  if (!checkMapper<MapCompressedString>(COMPRESSED_STRING, "Compressed string")) {
    success = false;
  }

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
