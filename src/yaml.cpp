#include "yaml.hpp"

#include "helper.hpp"

#include <iomanip>

namespace bml::detail {
  extern std::ostream &printUtf8String(std::ostream &os, std::u8string_view val, char escape);
}

namespace bml::yaml {
  std::string Options::indentation(bool firstMember) const {
    if (firstMember && inSequence) {
      // already handled by sequenceIndentation() of the parent sequence
      return "";
    }
    std::string indent(depth * 2U, ' ');
    if (!firstMember || depth > 0) {
      indent = "\n" + indent;
    }
    return indent;
  }

  std::string Options::sequenceIndentation() const {
    std::string indent(depth * 2U, ' ');
    if (depth > 0) {
      indent[(depth - 1U) * 2U] = '-';
      indent = "\n" + indent;
    }
    return indent;
  }

  static void prefixSpace(std::ostream &os, const Options &options) {
    if (options.prefixSpace) {
      os << ' ';
    }
  }

  std::ostream &print(std::ostream &os, const Options &options, std::monostate) {
    prefixSpace(os, options);
    return os << "null";
  }

  std::ostream &print(std::ostream &os, const Options &options, std::byte val) {
    prefixSpace(os, options);
    return os << "0x" << std::setfill('0') << std::hex << std::setw(2) << static_cast<unsigned>(val) << std::dec;
  }

  std::ostream &print(std::ostream &os, const Options &options, bool val) {
    prefixSpace(os, options);
    return os << (val ? "true" : "false");
  }

  std::ostream &print(std::ostream &os, const Options &options, uintmax_t val) {
    prefixSpace(os, options);
    return os << val;
  }

  std::ostream &print(std::ostream &os, const Options &options, intmax_t val) {
    prefixSpace(os, options);
    return os << val;
  }

  std::ostream &print(std::ostream &os, const Options &options, float val) {
    prefixSpace(os, options);
    return os << val;
  }

  std::ostream &print(std::ostream &os, const Options &options, double val) {
    prefixSpace(os, options);
    return os << val;
  }

  std::ostream &print(std::ostream &os, const Options &options, std::string_view val) {
    prefixSpace(os, options);
    // In YAML, single quotes in a single-quoted string are escaped by another single quote
    return os << std::quoted(val, '\'', '\'');
  }

  std::ostream &print(std::ostream &os, const Options &options, std::u8string_view val) {
    prefixSpace(os, options);
    // In YAML, single quotes in a single-quoted string are escaped by another single quote
    return bml::detail::printUtf8String(os, val, '\'');
  }

  std::ostream &YAMLTraits<bml::ByteRange>::print(std::ostream &os, const Options &options, const bml::ByteRange &val) {
    detail::printMember(os, options, true /* first member */, "offset", val.offset.num);
    detail::printMember(os, options, false /* second member */, "size", val.size.num);
    return os;
  }

  bool YAMLTraits<bml::ByteRange>::isEmpty(const bml::ByteRange &val) { return !val; }

} // namespace bml::yaml
