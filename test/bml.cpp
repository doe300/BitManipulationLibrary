#include "bml.hpp"
#include "common.hpp"
#include "test_helper.hpp"
#include "yaml.hpp"

#include "cpptest-main.h"

#include <array>
#include <iomanip>
#include <iterator>
#include <sstream>

using namespace bml;

BML_YAML_DEFINE_PRINT(Member, a, b, c)

struct TestHelper : public Test::Suite {
public:
  TestHelper() : Test::Suite("Helper") {
    TEST_ADD(TestHelper::testBits);
    TEST_ADD(TestHelper::testInvertBits);
    TEST_ADD(TestHelper::testEncodeExpGolomb);
    TEST_ADD(TestHelper::testDecodeExpGolomb);
    TEST_ADD(TestHelper::testEncodeSignedExpGolomb);
    TEST_ADD(TestHelper::testDecodeSignedExpGolomb);
    TEST_ADD(TestHelper::testEncodeFibonacci);
    TEST_ADD(TestHelper::testDecodeFibonacci);
    TEST_ADD(TestHelper::testEncodeSignedFibonacci);
    TEST_ADD(TestHelper::testDecodeSignedFibonacci);
    TEST_ADD(TestHelper::testHexString);
    TEST_ADD(TestHelper::testWriteBits);
  }

  void testBits() {
    static_assert(bits<uint8_t>() == 8);
    static_assert(bits<uint32_t>() == 32);
    TEST_ASSERT_EQUALS(1U, bits<bool>());
  }

  void testInvertBits() {
    TEST_ASSERT_EQUALS(0b1011U, invertBits(0b1101, 4_bits));
    TEST_ASSERT_EQUALS(0b01011U, invertBits(0b11010, 5_bits));
    TEST_ASSERT_EQUALS(0b101100U, invertBits(0b001101, 6_bits));
    TEST_ASSERT_EQUALS(0b00101100U, invertBits(0b00110100, 8_bits));
    TEST_ASSERT_EQUALS(0xABCU, invertBits(0x3D5U, 12_bits));
    TEST_ASSERT_EQUALS(0x55EU, invertBits(0x3D5U, 11_bits));
    TEST_ASSERT_EQUALS(0xAABBU, invertBits(0xDD55U, 2_bytes));
    TEST_ASSERT_EQUALS(0xAAABBBU, invertBits(0xDDD555U, 3_bytes));
    TEST_ASSERT_EQUALS(0x15780F54U, invertBits(0x55E03D5U, 29_bits));
    TEST_ASSERT_EQUALS(0xCCCCCCCCU, invertBits(0x33333333U, 4_bytes));
    TEST_ASSERT_EQUALS(0x33333333U, invertBits(0xCCCCCCCCU, 4_bytes));
    TEST_ASSERT_EQUALS(0xAAAAAAAABBBBBBBBU, invertBits(0xDDDDDDDD55555555U, 8_bytes));
  }

  void testEncodeExpGolomb() {
    static_assert(encodeExpGolomb(0U).value == 0b1);
    static_assert(encodeExpGolomb(0U).numBits == 1_bits);
    static_assert(encodeExpGolomb(1U).value == 0b010);
    static_assert(encodeExpGolomb(1U).numBits == 3_bits);
    static_assert(encodeExpGolomb(8U).value == 0b0001001);
    static_assert(encodeExpGolomb(8U).numBits.value() == 2 * 3 + 1);
    static_assert(encodeExpGolomb(17U).value == 0b000010010);
    static_assert(encodeExpGolomb(17U).numBits.value() == 2 * 4 + 1);
    static_assert(encodeExpGolomb(42U).value == 0b00000101011);
    static_assert(encodeExpGolomb(42U).numBits.value() == 2 * 5 + 1);
  }

  void testDecodeExpGolomb() {
    static_assert(decodeExpGolomb(0b1U) == 0);
    static_assert(decodeExpGolomb(0b010U) == 1);
    static_assert(decodeExpGolomb(0b0001001U) == 8);
    static_assert(decodeExpGolomb(0b000010010U) == 17);
    static_assert(decodeExpGolomb(0b00000101011U) == 42);
  }

  void testEncodeSignedExpGolomb() {
    static_assert(encodeSignedExpGolomb(0).value == 0b1);
    static_assert(encodeSignedExpGolomb(0).numBits == 1_bits);
    static_assert(encodeSignedExpGolomb(1).value == 0b010);
    static_assert(encodeSignedExpGolomb(1).numBits == 3_bits);
    static_assert(encodeSignedExpGolomb(8).value == 0b000010000);
    static_assert(encodeSignedExpGolomb(8).numBits.value() == 2 * 4 + 1);
    static_assert(encodeSignedExpGolomb(17).value == 0b00000100010);
    static_assert(encodeSignedExpGolomb(17).numBits.value() == 2 * 5 + 1);
    static_assert(encodeSignedExpGolomb(42).value == 0b0000001010100);
    static_assert(encodeSignedExpGolomb(42).numBits.value() == 2 * 6 + 1);
    static_assert(encodeSignedExpGolomb(-1).value == 0b011);
    static_assert(encodeSignedExpGolomb(-1).numBits == 3_bits);
    static_assert(encodeSignedExpGolomb(-8).value == 0b000010001);
    static_assert(encodeSignedExpGolomb(-8).numBits.value() == 2 * 4 + 1);
    static_assert(encodeSignedExpGolomb(-17).value == 0b00000100011);
    static_assert(encodeSignedExpGolomb(-17).numBits.value() == 2 * 5 + 1);
    static_assert(encodeSignedExpGolomb(-42).value == 0b0000001010101);
    static_assert(encodeSignedExpGolomb(-42).numBits.value() == 2 * 6 + 1);
  }

  void testDecodeSignedExpGolomb() {
    static_assert(decodeSignedExpGolomb(0b1U) == 0);
    static_assert(decodeSignedExpGolomb(0b010U) == 1);
    static_assert(decodeSignedExpGolomb(0b000010000U) == 8);
    static_assert(decodeSignedExpGolomb(0b00000100010U) == 17);
    static_assert(decodeSignedExpGolomb(0b0000001010100U) == 42);
    static_assert(decodeSignedExpGolomb(0b011U) == -1);
    static_assert(decodeSignedExpGolomb(0b000010001U) == -8);
    static_assert(decodeSignedExpGolomb(0b00000100011U) == -17);
    static_assert(decodeSignedExpGolomb(0b0000001010101U) == -42);
  }

  void testEncodeFibonacci() {
    TEST_ASSERT_EQUALS(0b11U, encodeFibonacci(1U).value);
    TEST_ASSERT_EQUALS(2_bits, encodeFibonacci(1U).numBits);
    TEST_ASSERT_EQUALS(0b11000U, encodeFibonacci(5U).value);
    TEST_ASSERT_EQUALS(5_bits, encodeFibonacci(5U).numBits);
    TEST_ASSERT_EQUALS(0b110001U, encodeFibonacci(9U).value);
    TEST_ASSERT_EQUALS(6_bits, encodeFibonacci(9U).numBits);
    TEST_ASSERT_EQUALS(0b1100001U, encodeFibonacci(14U).value);
    TEST_ASSERT_EQUALS(7_bits, encodeFibonacci(14U).numBits);
    TEST_ASSERT_EQUALS(0b1100010010U, encodeFibonacci(65U).value);
    TEST_ASSERT_EQUALS(10_bits, encodeFibonacci(65U).numBits);
    TEST_ASSERT_EQUALS(0b11000101000U, encodeFibonacci(107U).value);
    TEST_ASSERT_EQUALS(11_bits, encodeFibonacci(107U).numBits);
    TEST_ASSERT_EQUALS(0xD402A1520AAU, encodeFibonacci(1073741824U).value);
    TEST_ASSERT_EQUALS(44_bits, encodeFibonacci(1073741824U).numBits);
  }

  void testDecodeFibonacci() {
    TEST_ASSERT_EQUALS(1U, decodeFibonacci(0b11));
    TEST_ASSERT_EQUALS(5U, decodeFibonacci(0b11000));
    TEST_ASSERT_EQUALS(9U, decodeFibonacci(0b110001));
    TEST_ASSERT_EQUALS(14U, decodeFibonacci(0b1100001));
    TEST_ASSERT_EQUALS(65U, decodeFibonacci(0b1100010010));
    TEST_ASSERT_EQUALS(107U, decodeFibonacci(0b11000101000));
    TEST_ASSERT_EQUALS(1073741824U, decodeFibonacci(0xD402A1520AAU));
  }

  void testEncodeSignedFibonacci() {
    TEST_ASSERT_EQUALS(0b1101000U, encodeNegaFibonacci(-11).value);
    TEST_ASSERT_EQUALS(7_bits, encodeNegaFibonacci(-11).numBits);
    TEST_ASSERT_EQUALS(0b1100000U, encodeNegaFibonacci(-8).value);
    TEST_ASSERT_EQUALS(7_bits, encodeNegaFibonacci(-8).numBits);
    TEST_ASSERT_EQUALS(0b11000U, encodeNegaFibonacci(-3).value);
    TEST_ASSERT_EQUALS(5_bits, encodeNegaFibonacci(-3).numBits);
    TEST_ASSERT_EQUALS(0b110U, encodeNegaFibonacci(-1).value);
    TEST_ASSERT_EQUALS(3_bits, encodeNegaFibonacci(-1).numBits);
    TEST_ASSERT_EQUALS(0b11U, encodeNegaFibonacci(1).value);
    TEST_ASSERT_EQUALS(2_bits, encodeNegaFibonacci(1).numBits);
    TEST_ASSERT_EQUALS(0b1101U, encodeNegaFibonacci(3).value);
    TEST_ASSERT_EQUALS(4_bits, encodeNegaFibonacci(3).numBits);
    TEST_ASSERT_EQUALS(0b110101U, encodeNegaFibonacci(8).value);
    TEST_ASSERT_EQUALS(6_bits, encodeNegaFibonacci(8).numBits);
    TEST_ASSERT_EQUALS(0b11001001U, encodeNegaFibonacci(11).value);
    TEST_ASSERT_EQUALS(8_bits, encodeNegaFibonacci(11).numBits);
    TEST_ASSERT_EQUALS(0x302A94408154U, encodeNegaFibonacci(1073741824).value);
    TEST_ASSERT_EQUALS(46_bits, encodeNegaFibonacci(1073741824).numBits);
    TEST_ASSERT_EQUALS(0x1A80814A9401U, encodeNegaFibonacci(-1073741824).value);
    TEST_ASSERT_EQUALS(45_bits, encodeNegaFibonacci(-1073741824).numBits);
  }

  void testDecodeSignedFibonacci() {
    TEST_ASSERT_EQUALS(-11, decodeNegaFibonacci(0b1101000));
    TEST_ASSERT_EQUALS(-8, decodeNegaFibonacci(0b1100000));
    TEST_ASSERT_EQUALS(-3, decodeNegaFibonacci(0b11000));
    TEST_ASSERT_EQUALS(-1, decodeNegaFibonacci(0b110));
    TEST_ASSERT_EQUALS(1, decodeNegaFibonacci(0b11));
    TEST_ASSERT_EQUALS(3, decodeNegaFibonacci(0b1101));
    TEST_ASSERT_EQUALS(8, decodeNegaFibonacci(0b110101));
    TEST_ASSERT_EQUALS(11, decodeNegaFibonacci(0b11001001));
    TEST_ASSERT_EQUALS(1073741824, decodeNegaFibonacci(0x302A94408154U));
    TEST_ASSERT_EQUALS(-1073741824, decodeNegaFibonacci(0x1A80814A9401U));
  }

  void testHexString() {
    TEST_ASSERT_EQUALS("00000123", toHexString(0x123U, false));
    TEST_ASSERT_EQUALS("0x00000123", toHexString(0x123U, true));
    TEST_ASSERT_EQUALS("23", toHexString(uint8_t{0x23}, false));
    TEST_ASSERT_EQUALS("0x23", toHexString(uint8_t{0x23}, true));
    TEST_ASSERT_EQUALS("0123", toHexString(uintmax_t{0x123}, 2_bytes, false));
    TEST_ASSERT_EQUALS("0x0123", toHexString(uintmax_t{0x123}, 2_bytes, true));

    TEST_ASSERT_EQUALS("00000017", toHexString(23U, 4_bytes, false));
    TEST_ASSERT_EQUALS("0x00", toHexString(uint8_t{0}, 1_bytes, true));
  }

  void testWriteBits() {
    const auto writeBits = [](auto value, BitCount numBits) {
      std::stringstream ss;
      bml::writeBits(ss, value, numBits);
      return ss.str();
    };
    TEST_ASSERT_EQUALS("0b00101", writeBits(0x05, 5_bits));
    TEST_ASSERT_EQUALS("0x005", writeBits(0x05, 9_bits));
    TEST_ASSERT_EQUALS("0b1010101", writeBits(0x55, 7_bits));
    TEST_ASSERT_EQUALS("0x55", writeBits(0x55, 8_bits));
  }
};

namespace bml {
  std::ostream &operator<<(std::ostream &os, const Cache &cache) {
    return os << "{0x" << std::hex << std::setfill('0') << std::setw(2 * sizeof(std::uintmax_t)) << cache.value << ", "
              << std::dec << cache.size.bits() << '}';
  }
} // namespace bml

class TestCache : public Test::Suite {
public:
  TestCache() : Test::Suite("Cache") {
    TEST_ADD(TestCache::testFillCache);
    TEST_ADD(TestCache::testReadCache);
    TEST_ADD(TestCache::testFlushCache);
  }

  void testFillCache() {
    TEST_ASSERT_EQUALS((Cache{0x1234450000000000, 24_bits}), fillCacheHelper(Cache{}, 17_bits, {0x12, 0x34, 0x45}));
    TEST_ASSERT_EQUALS((Cache{0x1200000000000000, 8_bits}), fillCacheHelper(Cache{}, 3_bits, {0x12, 0x34, 0x45}));
    TEST_ASSERT_EQUALS((Cache{0x1234120000000000, 24_bits}),
                       fillCacheHelper(Cache{0x1234000000000000, 16_bits}, 17_bits, {0x12, 0x34, 0x45}))
    TEST_ASSERT_EQUALS((Cache{0xE000000000000000, 3_bits}),
                       fillCacheHelper(Cache{0xE000000000000000, 3_bits}, 3_bits, {}))
    TEST_ASSERT_EQUALS((Cache{0x1224680000000000, 16_bits + 7_bits}),
                       fillCacheHelper(Cache{0x1200000000000000, 7_bits}, 17_bits, {0x12, 0x34, 0x45}));
  }

  void testReadCache() {
    TEST_ASSERT_EQUALS(0x2468U, readCacheHelper(Cache{}, 17_bits, {0x12, 0x34, 0x45}).first);
    TEST_ASSERT_EQUALS((Cache{0x8A00000000000000, 7_bits}),
                       readCacheHelper(Cache{}, 17_bits, {0x12, 0x34, 0x45}).second);
    TEST_ASSERT_EQUALS(0U, readCacheHelper(Cache{}, 3_bits, {0x12, 0x34, 0x45}).first);
    TEST_ASSERT_EQUALS((Cache{0x9000000000000000, 5_bits}),
                       readCacheHelper(Cache{}, 3_bits, {0x12, 0x34, 0x45}).second);
    TEST_ASSERT_EQUALS(0x2468U, readCacheHelper(Cache{0x1234000000000000, 16_bits}, 17_bits, {0x12, 0x34, 0x45}).first);
    TEST_ASSERT_EQUALS((Cache{0x2400000000000000, 7_bits}),
                       readCacheHelper(Cache{0x1234000000000000, 16_bits}, 17_bits, {0x12, 0x34, 0x45}).second);
    TEST_ASSERT_EQUALS(0x7U, readCacheHelper(Cache{0xE000000000000000, 3_bits}, 3_bits, {}).first);
    TEST_ASSERT_EQUALS((Cache{0x0, 0_bits}), readCacheHelper(Cache{0xE000000000000000, 3_bits}, 3_bits, {}).second);

    TEST_ASSERT_EQUALS(0x2448U, readCacheHelper(Cache{0x1200000000000000, 7_bits}, 17_bits, {0x12, 0x34, 0x45}).first);
    TEST_ASSERT_EQUALS((Cache{0xD000000000000000, 6_bits}),
                       readCacheHelper(Cache{0x1200000000000000, 7_bits}, 17_bits, {0x12, 0x34, 0x45}).second);
  }

  void testFlushCache() {
    TEST_ASSERT_EQUALS(0x1234000000000000U, flushFullCacheBytesHelper(Cache{0x1234000000000000, 16_bits}).first);
    TEST_ASSERT_EQUALS((Cache{0x0, 0_bytes}), flushFullCacheBytesHelper(Cache{0x1234000000000000, 16_bits}).second);
    TEST_ASSERT_EQUALS(0x0U, flushFullCacheBytesHelper(Cache{0xE000000000000000, 3_bits}).first);
    TEST_ASSERT_EQUALS((Cache{0xE000000000000000, 3_bits}),
                       flushFullCacheBytesHelper(Cache{0xE000000000000000, 3_bits}).second);
    TEST_ASSERT_EQUALS(0x1200000000000000U, flushFullCacheBytesHelper(Cache{0x1234000000000000, 15_bits}).first);
    TEST_ASSERT_EQUALS((Cache{0x3400000000000000, 7_bits}),
                       flushFullCacheBytesHelper(Cache{0x1234000000000000, 15_bits}).second);
  }

private:
  static Cache fillCacheHelper(Cache cache, BitCount numBits, std::initializer_list<const uint8_t> input) {
    fillCache(
        cache, numBits,
        [it{input.begin()}](std::byte &nextByte) mutable {
          nextByte = static_cast<std::byte>(*it++);
          return true;
        },
        []() { throw; });
    return cache;
  }

  static std::pair<std::uintmax_t, Cache> readCacheHelper(Cache cache, BitCount numBits,
                                                          std::initializer_list<const uint8_t> input) {
    cache = fillCacheHelper(cache, numBits, input);
    auto cacheSize = cache.size;
    auto result = readFromCache(cache, numBits);
    if (cacheSize != (cache.size + numBits)) {
      throw;
    }
    return std::make_pair(result, cache);
  }

  static std::pair<std::uintmax_t, Cache> flushFullCacheBytesHelper(Cache cache) {
    Cache output{};
    flushFullCacheBytes(
        cache,
        [&output](std::byte nextByte) {
          if (output.size >= sizeof(std::uintmax_t) * 1_bytes) {
            return false;
          }
          std::uintmax_t tmp = static_cast<uint8_t>(nextByte);
          output.value |= tmp << (sizeof(std::uintmax_t) * 1_bytes - 1_bytes - output.size);
          output.size += 1_bytes;
          return true;
        },
        []() { throw; });
    return std::make_pair(output.value, cache);
  }
};

class TestSizes : public Test::Suite {
public:
  TestSizes() : Test::Suite("Sizes") {
    TEST_ADD(TestSizes::testBasic);
    TEST_ADD(TestSizes::testArithmetic);
    TEST_ADD(TestSizes::testShift);
    TEST_ADD(TestSizes::testFunctions);
  }

  void testBasic() {
    static_assert(!0_bytes);
    static_assert(1_bits);
    static_assert(1_bytes == 8_bits);
    static_assert(1_bytes != 7_bits);
    static_assert(1_bytes > 7_bits);
    TEST_ASSERT_EQUALS(7U, static_cast<std::size_t>(7_bits));
    TEST_ASSERT_EQUALS(17U, (17_bytes).value());
    TEST_ASSERT_FALSE(1_bytes < 7_bits);
  }

  void testArithmetic() {
    static_assert((1_bytes + 7_bits).value() == 15);
    static_assert(1_bytes - 7_bits == 1_bits);
    static_assert(1_bytes / 7_bits == 1);
    static_assert(7_bits / 1_bytes == 0);
    static_assert(1_bytes % 7_bits == 1_bits);
    static_assert(7_bits % 1_bytes == 7_bits);
    static_assert(1_bytes * 7 == 7_bytes);
    static_assert(7 * 1_bytes == 7_bytes);
    static_assert(1_bytes / 7 == 0_bytes);
    static_assert(7_bytes / 2 == 3_bytes);
    static_assert(7_bytes % 2 == 1);

    auto b = 17_bits;
    TEST_ASSERT_EQUALS(19_bits, b += 2_bits);
    TEST_ASSERT_EQUALS(27_bits, b += 1_bytes);
    TEST_ASSERT_EQUALS(11_bits, b -= 2_bytes);
    TEST_ASSERT_EQUALS(22_bits, b *= 2);
    TEST_ASSERT_EQUALS(21_bits, --b);
    TEST_ASSERT_EQUALS(22_bits, ++b);
  }

  void testShift() {
    static_assert((17 >> 0_bits) == 17);
    static_assert((17 >> 2_bits) == 4);
    static_assert((17 >> 7_bits) == 0);
    static_assert((17 << 0_bits) == 17);
    static_assert((17 << 2_bits) == 68);
    static_assert(7_bits < 1_bytes);
    static_assert(7_bytes > 1_bytes);

    uint32_t u = 0xFEAD;
    TEST_ASSERT_EQUALS(0xFEAU, u >>= 4_bits);
    TEST_ASSERT_EQUALS(0xFEA00U, u <<= 1_bytes);
  }

  void testFunctions() {
    static_assert((7_bytes).bits() == 56);
    static_assert((7_bits).mask() == 0x7F);
    static_assert((7_bytes).mask() == 0x00FFFFFFFFFFFFFF);
    static_assert((0_bits).mask() == 0);
    static_assert((1_bits).mask() == 0b1);
    static_assert((32_bits).mask() == 0xFFFFFFFF);
    static_assert((64_bits).mask() == 0xFFFFFFFFFFFFFFFF);
    static_assert((8_bytes).mask() == 0xFFFFFFFFFFFFFFFF);

    std::array<std::byte, 16> buf{};
    auto *ptr = buf.data() + 13_bytes;
    TEST_ASSERT_EQUALS(13, std::distance(buf.data(), ptr));

    TEST_ASSERT_EQUALS("0b", (0_bits).toString());
    TEST_ASSERT_EQUALS("17b", (17_bits).toString());
    TEST_ASSERT_EQUALS("2kb", (2048_bits).toString());
    TEST_ASSERT_EQUALS("2.1kb", (2148_bits).toString());
    TEST_ASSERT_EQUALS("325B", (325_bytes).toString());
    TEST_ASSERT_EQUALS("325MB", (340787200_bytes).toString());
    TEST_ASSERT_EQUALS("334.54MB", (350787200_bytes).toString());

    TEST_ASSERT_EQUALS(1U, (15_bits).divide<8>().num);
    TEST_ASSERT_EQUALS(2U, (16_bits).divide<8>().num);
    TEST_ASSERT_EQUALS(2U, (17_bits).divide<8>().num);
    TEST_ASSERT_EQUALS(5U, (15_bits).divide<3>().num);
    TEST_ASSERT_EQUALS(5U, (16_bits).divide<3>().num);
    TEST_ASSERT_EQUALS(8U, (2_bytes).divide<2>().num);
  }
};

struct Complex {
  uint32_t u;
  Member m;
  std::vector<uint32_t> nums;
  std::vector<Member> additional;

  std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
};

BML_YAML_DEFINE_PRINT(Complex, u, m, nums, additional)

class TestYAML : public Test::Suite {
public:
  TestYAML() : Test::Suite("YAML") {
    TEST_ADD(TestYAML::testSimple);
    TEST_ADD(TestYAML::testObject);
    TEST_ADD(TestYAML::testCollections);
    TEST_ADD(TestYAML::testComplex);
  }

  void testSimple() {
    using namespace std::string_literals;

    checkPrint(std::byte{0x11}, "0x11");
    checkPrint(false, "false");
    checkPrint(true, "true");
    checkPrint(112U, "112");
    checkPrint(-2300, "-2300");
    checkPrint(-2300.3F, "-2300.3");
    checkPrint(42.0, "42");
    checkPrint("Foo Bar"s, "'Foo Bar'");
    checkPrint(u8"Foo Bar ⠁⠃"s, "'Foo Bar ⠁⠃'");
    checkPrint(std::optional<int>{}, "null");
    checkPrint(std::optional<int>{15}, "15");

    {
      // Some systems append the nanoseconds, others don't
      std::stringstream ss;
      bml::yaml::print(ss, bml::yaml::Options{}, std::chrono::utc_seconds{});
      TEST_ASSERT(ss.str().starts_with("1970-01-01 00:00:00"));
    }
  }

  void testObject() {
    Member m{13, 3000, -3};
    checkPrint(m, R"(a: 13
b: 3000
c: -3)");
  }

  void testCollections() {
    checkPrint(std::vector<std::byte>{std::byte{0x01}, std::byte{0x03}, std::byte{0xFF}}, "[0x01, 0x03, 0xff, ]");
    checkPrint(std::vector<uint8_t>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
               "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, ]");
    checkPrint(std::vector<uint8_t>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20},
               "(20 entries)");

    checkPrint(std::vector<Member>{Member{13, 3000, -3}, Member{1, 3, -3}, Member{42, 42, 42}}, R"(
- a: 13
  b: 3000
  c: -3
- a: 1
  b: 3
  c: -3
- a: 42
  b: 42
  c: 42)");
  }

  void testComplex() {
    Complex c{};
    c.u = 42;
    c.m = Member{13, 3000, -3};
    c.nums = {13, 17, 1};
    c.additional = {Member{13, 3000, -3}, Member{1, 3, -3}};

    checkPrint(c, R"(u: 42
m:
  a: 13
  b: 3000
  c: -3
nums: [13, 17, 1, ]
additional:
  - a: 13
    b: 3000
    c: -3
  - a: 1
    b: 3
    c: -3)");
  }

private:
  template <typename T>
  void checkPrint(const T &val, std::string_view expectedString) {
    std::stringstream ss;
    bml::yaml::print(ss, bml::yaml::Options{}, val);
    TEST_ASSERT_EQUALS(expectedString, ss.str());
  }
};

int main(int argc, char **argv) {
  registerBinaryMapTests();
  registerTypeTests();
  Test::registerSuite(Test::newInstance<TestHelper>, "helper");
  Test::registerSuite(Test::newInstance<TestCache>, "cache");
  Test::registerSuite(Test::newInstance<TestSizes>, "sizes");
  Test::registerSuite(Test::newInstance<TestYAML>, "yaml");
  return Test::runSuites(argc, argv);
}
