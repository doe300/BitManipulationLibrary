#include "types.hpp"

#include "common.hpp"

#include <array>
#ifdef _MSC_VER
#include <cuchar>
#endif
#include <string_view>

namespace bml {

  std::ostream &operator<<(std::ostream &os, const UnicodeCodePoint &codePoint) {
    if (codePoint.value == 0) {
      return os << "\\0";
    }
#ifdef _MSC_VER
    std::mbstate_t state{};
    std::array<char, MB_LEN_MAX> tmp{};
    std::size_t rc = std::c32rtomb(tmp.data(), codePoint, &state);
    if (rc == static_cast<std::size_t>(-1)) {
      return os << '?';
    }
    return os << std::string_view(tmp.data(), rc);
#else
    // Linux and macOS use (most likely) UTF-8 for their string encoding, so just assume that.
    // For any other platform we don't know the necessary conversion anyway, so also treat as UTF-8.
    auto utf8String = toUtf8String(codePoint.value);
    return os << std::string_view(reinterpret_cast<const char *>(utf8String.data()), utf8String.size());
#endif
  }
} // namespace bml
