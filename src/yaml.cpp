#include "yaml.hpp"

#include <iomanip>

namespace bml::detail {
  extern std::ostream &printUtf8String(std::ostream &os, std::u8string_view val);
}

namespace bml::yaml {
  std::string Options::indentation() const { return std::string(depth * 2U, ' '); }
  std::size_t Options::truncateSequenceLimit() const noexcept { return 16U; }

  std::ostream &print(std::ostream &os, const Options & /* options */, std::byte val) {
    return os << "0x" << std::setfill('0') << std::hex << std::setw(2) << static_cast<unsigned>(val) << std::dec;
  }

  std::ostream &print(std::ostream &os, const Options & /* options */, bool val) {
    return os << (val ? "true" : "false");
  }
  std::ostream &print(std::ostream &os, const Options & /* options */, uintmax_t val) { return os << val; }
  std::ostream &print(std::ostream &os, const Options & /* options */, intmax_t val) { return os << val; }
  std::ostream &print(std::ostream &os, const Options & /* options */, float val) { return os << val; }
  std::ostream &print(std::ostream &os, const Options & /* options */, double val) { return os << val; }
  std::ostream &print(std::ostream &os, const Options & /* options */, std::string_view val) {
    return os << '\'' << val << '\'';
  }

  std::ostream &print(std::ostream &os, const Options & /* options */, std::u8string_view val) {
    return bml::detail::printUtf8String(os, val);
  }
} // namespace bml::yaml
