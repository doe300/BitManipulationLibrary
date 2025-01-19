#include "print.hpp"
#include "sizes.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <iomanip>
#ifdef _MSC_VER
#include <Windows.h>
#include <cuchar>
#endif

namespace bml::detail {

  static std::string toSizeString(std::size_t value, char postfix) {
    static constexpr auto giga = 1024ULL * 1024ULL * 1024ULL;
    static constexpr auto mega = 1024ULL * 1024ULL;
    static constexpr auto kilo = 1024ULL;

    const std::vector<std::pair<std::size_t, char>> MAGNITUDES{{giga, 'G'}, {mega, 'M'}, {kilo, 'k'}};
    std::array<char, 128> buffer{};

    // integer multiples
    for (const auto &magnitude : MAGNITUDES) {
      if (value >= magnitude.first && (value % magnitude.first) == 0) {
        if (auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value / magnitude.first);
            result.ec == std::errc{}) {
          *result.ptr++ = magnitude.second;
          *result.ptr++ = postfix;
          return std::string(buffer.data(), result.ptr);
        }
      }
    }
    // fractional multiples
    for (const auto &magnitude : MAGNITUDES) {
      if (value > magnitude.first) {
        auto tmp = static_cast<double>(value) / static_cast<double>(magnitude.first);
        tmp = std::round(tmp * 100.0f) / 100.0f;
        if (auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), tmp, std::chars_format::fixed);
            result.ec == std::errc{}) {
          *result.ptr++ = magnitude.second;
          *result.ptr++ = postfix;
          return std::string(buffer.data(), result.ptr);
        }
      }
    }

    if (auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value); result.ec == std::errc{}) {
      *result.ptr++ = postfix;
      return std::string(buffer.data(), result.ptr);
    }
    return std::to_string(value) + postfix;
  }

  template <>
  template <>
  std::string SizeType<1>::toString<1>() const {
    return toSizeString(num, 'b');
  }

  template <>
  template <>
  std::string SizeType<8>::toString<8>() const {
    return toSizeString(num, 'B');
  }

  std::ostream &operator<<(std::ostream &os, const PrintView<std::byte> &view) {
    return os << "0x" << std::setfill('0') << std::hex << std::setw(2) << static_cast<unsigned>(view.value) << std::dec;
  }

  std::ostream &operator<<(std::ostream &os, const PrintView<std::span<const std::byte>> &view) {
    os << view.value.size() << " [";
    for (auto entry : view.value) {
      os << "0x" << std::setfill('0') << std::hex << std::setw(2) << static_cast<unsigned>(entry) << ", ";
    }
    return os << std::dec << ']';
  }

  static const bool NATIVE_UTF8 = [] {
#ifdef _MSC_VER
    return GetACP() == CP_UTF8;
#else
    // Linux and macOS use (most likely) UTF-8 for their string encoding, so just assume that.
    // For any other platform we don't know the necessary conversion anyway, so also treat as UTF-8.
    return true;
#endif
  }();

  std::ostream &printUtf8String(std::ostream &os, std::u8string_view val) {
    if (NATIVE_UTF8) {
      return os << '\'' << std::string_view(reinterpret_cast<const char *>(val.data()), val.size()) << '\'';
    }
#ifdef _MSC_VER
    std::wstring tmp(val.size() * 2U, L'\0');
    auto numChars = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char *>(val.data()),
                                        static_cast<int>(val.size()), &tmp.front(), static_cast<int>(tmp.size()));
    tmp.resize(numChars);
    std::string result(val.size() * 2U, '\0');
    numChars = WideCharToMultiByte(CP_ACP, 0, tmp.data(), static_cast<int>(tmp.size()), &result.front(),
                                   static_cast<int>(result.size()), nullptr, nullptr);
    result.resize(numChars);
    return os << '\'' << result << '\'';
#else
    return os << "(unknown encoding)";
#endif
  }

  std::ostream &operator<<(std::ostream &os, const PrintView<std::u8string> &view) {
    return printUtf8String(os, view.value);
  }

} // namespace bml::detail
