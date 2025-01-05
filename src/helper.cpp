#include "helper.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <bitset>
#include <cmath>
#include <iomanip>
#include <numbers>

namespace bml {

  static constexpr std::uintmax_t invertBitsInByte(std::uintmax_t value) noexcept {
    // Adapted from https://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64Bits
    return (((value * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32U) & (1_bytes).mask();
  }

  static_assert(invertBitsInByte(0b10110010) == 0b01001101);

  static constexpr std::uintmax_t invert32Bits(std::uintmax_t value) noexcept {
    // Adapted from https://graphics.stanford.edu/~seander/bithacks.html#ReverseParallel

    // swap odd and even bits
    value = ((value >> 1U) & 0x55555555U) | ((value & 0x55555555U) << 1U);
    // swap consecutive pairs
    value = ((value >> 2u) & 0x33333333U) | ((value & 0x33333333U) << 2U);
    // swap nibbles ...
    value = ((value >> 4U) & 0x0F0F0F0FU) | ((value & 0x0F0F0F0FU) << 4U);
    // swap bytes
    value = ((value >> 8U) & 0x00FF00FFU) | ((value & 0x00FF00FFU) << 8U);
    // swap 2-byte long pairs
    value = (value >> 16U) | (value << 16U);
    return value & (4_bytes).mask();
  }

  static_assert(invert32Bits(0x33333333U) == 0xCCCCCCCCU);

  std::uintmax_t invertBits(std::uintmax_t value, BitCount numBits) {
    if (numBits <= 1_bytes) {
      return (invertBitsInByte(value) >> (1_bytes - numBits)) & numBits.mask();
    } else if (numBits <= 4_bytes) {
      return (invert32Bits(value) >> (4_bytes - numBits)) & numBits.mask();
    } else if (numBits <= 8_bytes) {
      auto tmp =
          invert32Bits((value >> 4_bytes) & (4_bytes).mask()) | (invert32Bits(value & (4_bytes).mask()) << 4_bytes);
      return (tmp >> (8_bytes - numBits)) & numBits.mask();
    }
    // generic case
    // Adapted from https://graphics.stanford.edu/~seander/bithacks.html#ReverseParallel
    std::uintmax_t mask = std::numeric_limits<std::uintmax_t>::max();
    while ((numBits = numBits / 2).value() > 0) {
      mask ^= (mask << numBits);
      value = ((value >> numBits) & mask) | ((value << numBits) & ~mask);
    }
    return value;
  }

  static constexpr std::string toHexStringInner(std::uintmax_t value, ByteCount typeSize, bool withPrefix) noexcept {
    std::string mapping = "0123456789ABCDEF";
    std::string s(typeSize.value() * 2U + (withPrefix ? 2U : 0U), '0');
    if (withPrefix) {
      s[1] = 'x';
    }

    auto pos = s.rbegin();
    while (value && pos < s.rend()) {
      *pos++ = mapping[value % 16];
      value /= 16U;
    }

    return s;
  }

  std::string toHexString(std::uintmax_t value, ByteCount typeSize, bool withPrefix) noexcept {
    return toHexStringInner(value, typeSize, withPrefix);
  }

  static constexpr auto FIBONACCI_NUMBERS = []() {
    std::array<uint32_t, 48> numbers;
    numbers[0] = 0;
    numbers[1] = 1;
    for (std::size_t i = 2; i < numbers.size(); ++i) {
      if (numbers[i - 1U] > (std::numeric_limits<uint32_t>::max() / 2U)) {
        throw;
      }
      numbers[i] = numbers[i - 2U] + numbers[i - 1U];
    }
    return numbers;
  }();
  static_assert(FIBONACCI_NUMBERS[17] == 1597);
  static_assert(FIBONACCI_NUMBERS[47] == 2971215073U);

  static constexpr auto NEGAFIBONACCI_NUMBERS = []() {
    std::array<int32_t, 47> numbers;
    for (uint32_t i = 0; i < numbers.size(); ++i) {
      numbers[i] = ((i % 2 == 0) ? -1 : 1) * static_cast<int32_t>(FIBONACCI_NUMBERS[i]);
    }
    return numbers;
  }();
  static_assert(NEGAFIBONACCI_NUMBERS[6] == -8);
  static_assert(NEGAFIBONACCI_NUMBERS[7] == 13);
  static_assert(NEGAFIBONACCI_NUMBERS[17] == 1597);
  static_assert(NEGAFIBONACCI_NUMBERS[46] == -1836311903);

  EncodedValue<std::uintmax_t> encodeFibonacci(uint32_t value) noexcept {
    std::bitset<FIBONACCI_NUMBERS.size()> bits{};
    // skip first 0 and 1
    auto start = FIBONACCI_NUMBERS.begin() + 2;
    auto end = FIBONACCI_NUMBERS.end();
    if (!value) {
      bits.set(0);
    }
    auto numBits = 0_bits;
    while (value) {
      auto fibIt = std::lower_bound(start, end, value);
      if (fibIt == end || *fibIt > value) {
        --fibIt;
      }
      auto pos = static_cast<std::size_t>(std::distance(start, fibIt));
      numBits = std::max(BitCount{pos + 1U}, numBits);
      bits.set(pos);
      value -= *fibIt;
      end = fibIt;
    }
    // trailing 1-bit
    bits.set(numBits.value());
    ++numBits;
    return {bits.to_ullong(), numBits};
  }

  uint32_t decodeFibonacci(std::uintmax_t value) noexcept {
    uint32_t result = 0;

    // Remove trailing 1-bit
    if (auto highBit = std::bit_floor(value)) {
      value -= highBit;
    }
    while (value) {
      auto highBit = std::bit_floor(value);
      auto pos = static_cast<uint32_t>(std::countr_zero(highBit));
      result += FIBONACCI_NUMBERS[pos + 2U /* skip first 0 and 1 */];
      value -= highBit;
    }
    return result;
  }

  EncodedValue<std::uintmax_t> encodeNegaFibonacci(int32_t value) {
    static constexpr auto GOLDEN_RATIO = std::numbers::phi_v<float>;
    std::bitset<NEGAFIBONACCI_NUMBERS.size()> bits{};

    const auto requiresLargerValue = [](int32_t val, int32_t highestValue) {
      auto limit = static_cast<int32_t>(std::ceil(GOLDEN_RATIO * static_cast<float>(highestValue)));
      return ((highestValue < 0 && val <= limit) || (highestValue > 0 && val >= limit));
    };

    auto startIndex =
        std::distance(FIBONACCI_NUMBERS.begin(), std::lower_bound(FIBONACCI_NUMBERS.begin(), FIBONACCI_NUMBERS.end(),
                                                                  static_cast<uint32_t>(std::abs(value))));
    startIndex = std::min(startIndex + 1, static_cast<std::ptrdiff_t>(FIBONACCI_NUMBERS.size() - 1U));
    auto numBits = 0_bits;
    for (auto i = startIndex; i > 0; --i) {
      auto index = static_cast<uint32_t>(i);
      const auto negafibonacciNumber = NEGAFIBONACCI_NUMBERS[index];
      if (value == 0) {
        break;
      } else if ((negafibonacciNumber < 0 && value <= negafibonacciNumber) ||
                 (negafibonacciNumber > 0 && value >= negafibonacciNumber)) {
        bits.set(index - 1U /* skip first 0 */);
        numBits = std::max(BitCount{static_cast<uint32_t>(i)}, numBits);
        value -= negafibonacciNumber;
        // Next bit cannot be set, so can be skipped
        --i;
      } else if (i > 2 && requiresLargerValue(value, NEGAFIBONACCI_NUMBERS[index - 2U])) {
        // Initially possibly select the Negafibonacci number with larger absolute value if necessary, i.e. the
        // Negafibonacci numbers with smaller absolute values (and same sign) can never reach the value, e.g.:
        // 11 = +13 -3 +1.
        bits.set(index - 1U /* skip first 0 */);
        numBits = std::max(BitCount{static_cast<uint32_t>(i)}, numBits);
        value -= negafibonacciNumber;
        // Next bit cannot be set, so can be skipped
        --i;
      }
    }

    // trailing 1-bit
    bits.set(numBits.value());
    ++numBits;
    return {bits.to_ullong(), numBits};
  }

  int32_t decodeNegaFibonacci(std::uintmax_t value) {
    int32_t result = 0;

    // Remove trailing 1-bit
    if (auto highBit = std::bit_floor(value)) {
      value -= highBit;
    }
    while (value) {
      auto highBit = std::bit_floor(value);
      auto pos = static_cast<uint32_t>(std::countr_zero(highBit));
      result += NEGAFIBONACCI_NUMBERS[pos + 1U /* skip first 0 */];
      value -= highBit;
    }
    return result;
  }

  template <typename T>
  static void writeBitsInner(std::ostream &os, T value, BitCount numBits) {
    if (numBits < 8_bits) {
      os << "0b";
      for (BitCount i{}; i < numBits; ++i) {
        auto bit = (value >> (numBits - i - 1_bits)) & 0x1U;
        os << bit;
      }
    } else {
      auto numNibbles = static_cast<int32_t>(numBits / 4_bits + ((numBits % 4) != 0));
      os << "0x" << std::hex << std::setfill('0') << std::setw(numNibbles) << value << std::dec;
    }
  }

  void writeBits(std::ostream &os, std::uintmax_t value, BitCount numBits) { writeBitsInner(os, value, numBits); }
  void writeBits(std::ostream &os, std::intmax_t value, BitCount numBits) { writeBitsInner(os, value, numBits); }
} // namespace bml
