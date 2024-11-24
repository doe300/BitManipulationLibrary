#include "helper.hpp"

#include <iomanip>

namespace bml {

  static constexpr std::string toHexStringInner(std::uintmax_t value, ByteCount typeSize, bool withPrefix) noexcept {
    std::string mapping = "0123456789ABCDEF";
    std::string s(typeSize.value() * 2 + (withPrefix * 2), '0');
    if (withPrefix)
      s[1] = 'x';

    auto pos = s.rbegin();
    while (value && pos < s.rend()) {
      *pos++ = mapping[value % 16];
      value /= 16;
    }

    return s;
  }

  std::string toHexString(std::uintmax_t value, ByteCount typeSize, bool withPrefix) noexcept {
    return toHexStringInner(value, typeSize, withPrefix);
  }

  template <typename T>
  static void writeBitsInner(std::ostream &os, T value, BitCount numBits) {
    if (numBits < 8_bits) {
      os << "0b";
      for (BitCount i{}; i < numBits; ++i) {
        auto bit = (value >> (numBits - i - 1_bits)) & 0x1;
        os << bit;
      }
    } else {
      auto numNibbles = numBits / 4_bits + ((numBits % 4) != 0);
      os << "0x" << std::hex << std::setfill('0') << std::setw(numNibbles) << value << std::dec;
    }
  }

  void writeBits(std::ostream &os, std::uintmax_t value, BitCount numBits) { writeBitsInner(os, value, numBits); }
  void writeBits(std::ostream &os, std::intmax_t value, BitCount numBits) { writeBitsInner(os, value, numBits); }
} // namespace bml
