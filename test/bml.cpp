#include "bml.hpp"
#include "common.hpp"

#include "cpptest-main.h"

#include <algorithm>
#include <iomanip>
#include <memory>
#include <span>
#include <sstream>
#include <vector>

using namespace bml;

enum class Enum : uint8_t { FOO = 1, BAR = 17, BAZ = 32 };
enum class LargeEnum : uint32_t { FOO = 1, BAR = 17, BAZ = 32 };

struct NonTrival {
public:
  int foo;

private:
  float bar;

public:
  std::unique_ptr<int> baz;

protected:
  char c;
};

class Member {
public:
  constexpr Member() noexcept = default;
  constexpr Member(uint8_t val1, uint16_t val2, int8_t val3) noexcept : a(val1), b(val2), c(val3) {}

  static constexpr BitCount minNumBits() noexcept { return 4_bytes; }
  static constexpr BitCount maxNumBits() noexcept {
    // Just for testing purposes
    return 5_bytes;
  }
  constexpr BitCount numBits() const noexcept { return 4_bytes; }

  void read(BitReader &reader) {
    bml::read(reader, a);
    bml::read(reader, b);
    bml::read(reader, c);
  }

  void write(BitWriter &writer) const {
    bml::write(writer, a);
    bml::write(writer, b);
    bml::write(writer, std::bit_cast<uint8_t>(c));
  }

  constexpr bool operator==(const Member &other) const noexcept { return a == other.a && b == other.b && c == other.c; }

  friend std::ostream &operator<<(std::ostream &os, const Member &member) {
    return os << '{' << static_cast<uint32_t>(member.a) << ", " << member.b << ", " << static_cast<int32_t>(member.c)
              << '}';
  }

private:
  uint8_t a;
  uint16_t b;
  int8_t c;
};

struct POD {
  int32_t a;
  uint32_t foo;
  char c;

  BML_DEFINE_PRINT(POD, a, foo, c)

  // These are required for testing
  bool operator==(const POD &) const noexcept = default;
};

struct DynamicallySizedDestructurable {
  uint8_t u;
  PrefixString<8> s;

  BML_DEFINE_PRINT(DynamicallySizedDestructurable, u, s)

  // These are required for testing
  bool operator==(const DynamicallySizedDestructurable &) const noexcept = default;
};
static_assert(BitField<DynamicallySizedDestructurable>);

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

struct TestBase : public Test::Suite {
protected:
  TestBase(const std::string &name) : Test::Suite(name) {}

  static std::vector<std::byte> toBytes(std::initializer_list<uint8_t> list) {
    std::vector<std::byte> vec{};
    vec.resize(list.size());
    std::transform(list.begin(), list.end(), vec.begin(), [](uint8_t val) { return std::bit_cast<std::byte>(val); });
    return vec;
  }

  template <typename T>
  void checkRead(const T &expectedValue, std::span<const std::byte> data, BitCount numBits = ByteCount{sizeof(T)})
    requires(Readable<T>)
  {
    BitReader reader{data};
    T value{};
    TEST_THROWS_NOTHING(bml::read(reader, value));
    TEST_ASSERT_EQUALS(numBits, reader.position());
    TEST_ASSERT_EQUALS(expectedValue, value);
  }

  template <typename T>
  void checkWrite(const T &value, std::span<const std::byte> expectedData, BitCount numBits = ByteCount{sizeof(T)})
    requires(Writeable<T>)
  {
    std::vector<std::byte> data(expectedData.size());
    BitWriter writer{data};
    TEST_THROWS_NOTHING(bml::write(writer, value));
    TEST_ASSERT_EQUALS(numBits, writer.position());
    writer.fillToAligment(1_bytes, 0);
    TEST_ASSERT_EQUALS(expectedData, data);
  }

  template <typename T>
  void checkSkip(std::span<const std::byte> data, BitCount numBits = ByteCount{sizeof(T)})
    requires(Skipable<T>)
  {
    BitReader reader{data};
    TEST_THROWS_NOTHING(bml::skip<T>(reader));
    TEST_ASSERT_EQUALS(numBits, reader.position());
  }

  template <typename T>
  void checkCopy(std::span<const std::byte> expectedData, BitCount numBits = ByteCount{sizeof(T)})
    requires(Copyable<T>)
  {
    BitReader reader{expectedData};
    std::vector<std::byte> data(expectedData.size());
    BitWriter writer{data};
    TEST_THROWS_NOTHING(bml::copy<T>(reader, writer));
    TEST_ASSERT_EQUALS(numBits, reader.position());
    TEST_ASSERT_EQUALS(numBits, writer.position());
    writer.fillToAligment(1_bytes, 0);
    TEST_ASSERT_EQUALS(expectedData, data);
  }

  template <typename T>
  void checkPrint(const T &val, std::string_view expectedVal)
    requires(Printable<T>)
  {
    std::stringstream ss{};
    ss << printView(val);
    TEST_ASSERT_EQUALS(expectedVal, ss.str());
  }

  template <typename T>
  void checkSize(const T &val, BitCount expectedSize, bool fixedSize)
    requires(Sized<T>)
  {
    TEST_ASSERT(bml::minNumBits<T>() <= expectedSize);
    TEST_ASSERT_EQUALS(expectedSize, bml::numBits(val));
    TEST_ASSERT(bml::maxNumBits<T>() >= expectedSize);
    if (fixedSize) {
      TEST_ASSERT_EQUALS(bml::minNumBits<T>(), bml::maxNumBits<T>());
    } else {
      TEST_ASSERT_NOT_EQUALS(bml::minNumBits<T>(), bml::maxNumBits<T>());
    }
  }
};

class TestIO : public TestBase {
public:
  TestIO() : TestBase("IO") {
    TEST_ADD(TestIO::testBool);
    TEST_ADD(TestIO::testByte);
    TEST_ADD(TestIO::testChar);
    TEST_ADD(TestIO::testIntegral);
    TEST_ADD(TestIO::testEnum);
    TEST_ADD(TestIO::testNonTrivial);
    TEST_ADD(TestIO::testMemberFunctions);
    TEST_ADD(TestIO::testPOD);
    TEST_ADD(TestIO::testDynamicallySizedDestructurable);
    TEST_ADD(TestIO::testTuple);
    TEST_ADD(TestIO::testVariant);
    TEST_ADD(TestIO::testRawBytes);
    TEST_ADD(TestIO::testSizes);
    TEST_ADD(TestIO::testReadOptional);
  }

  void testBool() {
    const auto DATA = toBytes({0xD0});
    const std::array<bool, 4> INPUT{true, true, false, true};

    checkRead(true, DATA, 1_bits);
    checkRead(INPUT, DATA, 4_bits);
    static_assert(!Readable<std::optional<bool>>);
    static_assert(!Readable<std::unique_ptr<bool>>);
    static_assert(!Readable<std::vector<bool>>);

    checkWrite(true, toBytes({0x80}), 1_bits);
    checkWrite(INPUT, DATA, 4_bits);
    checkWrite(std::optional<bool>{}, {}, 0_bits);
    checkWrite(std::optional<bool>{true}, toBytes({0x80}), 1_bits);
    checkWrite(std::unique_ptr<bool>{}, {}, 0_bits);
    checkWrite(std::make_unique<bool>(true), toBytes({0x80}), 1_bits);
    checkWrite(std::vector<bool>{}, {}, 0_bits);
    checkWrite(std::vector<bool>(INPUT.begin(), INPUT.end()), DATA, 4_bits);

    checkSkip<bool>(DATA, 1_bits);
    checkSkip<std::array<bool, 4>>(DATA, 4_bits);
    static_assert(!Skipable<std::optional<bool>>);
    static_assert(!Skipable<std::unique_ptr<bool>>);
    static_assert(!Skipable<std::vector<bool>>);

    checkCopy<bool>(toBytes({0x80}), 1_bits);
    checkCopy<std::array<bool, 4>>(DATA, 4_bits);
    static_assert(!Copyable<std::optional<bool>>);
    static_assert(!Copyable<std::unique_ptr<bool>>);
    static_assert(!Copyable<std::vector<bool>>);

    checkPrint(true, "1");
    checkPrint(INPUT, "4 [1, 1, 0, 1, ]");
    checkPrint(std::optional<bool>{}, "(none)");
    checkPrint(std::optional<bool>{true}, "1");
    checkPrint(std::unique_ptr<bool>{}, "(none)");
    checkPrint(std::make_unique<bool>(true), "1");
    checkPrint(std::vector<bool>{}, "0 []");
    checkPrint(std::vector<bool>(INPUT.begin(), INPUT.end()), "4 [1, 1, 0, 1, ]");

    checkSize(true, 1_bits, true);
    checkSize(INPUT, 4_bits, true);
    checkSize(std::optional<bool>{}, 0_bits, false);
    checkSize(std::optional<bool>{true}, 1_bits, false);
    checkSize(std::unique_ptr<bool>{}, 0_bits, false);
    checkSize(std::make_unique<bool>(true), 1_bits, false);
    checkSize(std::vector<bool>{}, 0_bits, false);
    checkSize(std::vector<bool>(INPUT.begin(), INPUT.end()), 4_bits, false);
  }

  void testByte() {
    const std::array<std::byte, 4> INPUT{std::byte{0x11}, std::byte{0x42}, std::byte{0x17}, std::byte{0x00}};
    const std::span DATA{INPUT};

    checkRead(std::byte{0x11}, DATA);
    checkRead(INPUT, DATA);
    static_assert(!Readable<std::optional<std::byte>>);
    static_assert(!Readable<std::unique_ptr<std::byte>>);
    static_assert(!Readable<std::vector<std::byte>>);

    checkWrite(std::byte{0x11}, DATA.subspan(0, 1));
    checkWrite(INPUT, DATA);
    checkWrite(std::optional<std::byte>{}, {}, 0_bits);
    checkWrite(std::optional<std::byte>{std::byte{0x11}}, DATA.subspan(0, 1), 1_bytes);
    checkWrite(std::unique_ptr<std::byte>{}, {}, 0_bits);
    checkWrite(std::make_unique<std::byte>(std::byte{0x11}), DATA.subspan(0, 1), 1_bytes);
    checkWrite(std::vector<std::byte>{}, {}, 0_bits);
    checkWrite(std::vector<std::byte>{INPUT.begin(), INPUT.end()}, DATA, 4_bytes);

    checkSkip<std::byte>(DATA);
    checkSkip<std::array<std::byte, 4>>(DATA);
    static_assert(!Skipable<std::optional<std::byte>>);
    static_assert(!Skipable<std::unique_ptr<std::byte>>);
    static_assert(!Skipable<std::vector<std::byte>>);

    checkCopy<std::byte>(DATA.subspan(0, 1));
    checkCopy<std::array<std::byte, 4>>(DATA);
    static_assert(!Copyable<std::optional<std::byte>>);
    static_assert(!Copyable<std::unique_ptr<std::byte>>);
    static_assert(!Copyable<std::vector<std::byte>>);

    checkPrint(std::byte{0x11}, "0x11");
    checkPrint(INPUT, "4 [0x11, 0x42, 0x17, 0x00, ]");
    checkPrint(std::optional<std::byte>{}, "(none)");
    checkPrint(std::optional<std::byte>{std::byte{0x11}}, "0x11");
    checkPrint(std::unique_ptr<std::byte>{}, "(none)");
    checkPrint(std::make_unique<std::byte>(std::byte{0x11}), "0x11");
    checkPrint(std::vector<std::byte>{}, "0 []");
    checkPrint(std::vector<std::byte>{INPUT.begin(), INPUT.end()}, "4 [0x11, 0x42, 0x17, 0x00, ]");

    checkSize(std::byte{0x11}, 1_bytes, true);
    checkSize(INPUT, 4 * 1_bytes, true);
    checkSize(std::optional<std::byte>{}, 0_bytes, false);
    checkSize(std::optional<std::byte>{std::byte{0x11}}, 1_bytes, false);
    checkSize(std::unique_ptr<std::byte>{}, 0_bytes, false);
    checkSize(std::make_unique<std::byte>(std::byte{0x11}), 1_bytes, false);
    checkSize(std::vector<std::byte>{}, 0_bytes, false);
    checkSize(std::vector<std::byte>{INPUT.begin(), INPUT.end()}, 4 * 1_bytes, false);
  }

  void testChar() {
    const std::array<char, 4> INPUT{'F', 'o', 'b', 's'};
    const std::span DATA{INPUT};

    checkRead('F', std::as_bytes(DATA));
    checkRead(INPUT, std::as_bytes(DATA));
    static_assert(!Readable<std::optional<char>>);
    static_assert(!Readable<std::unique_ptr<char>>);
    static_assert(!Readable<std::vector<char>>);

    checkWrite('F', std::as_bytes(DATA.subspan(0, 1)));
    checkWrite(INPUT, std::as_bytes(DATA));
    checkWrite(std::optional<char>{}, {}, 0_bits);
    checkWrite(std::optional<char>{'F'}, std::as_bytes(DATA.subspan(0, 1)), 1_bytes);
    checkWrite(std::unique_ptr<char>{}, {}, 0_bits);
    checkWrite(std::make_unique<char>('F'), std::as_bytes(DATA.subspan(0, 1)), 1_bytes);
    checkWrite(std::vector<char>{}, {}, 0_bits);
    checkWrite(std::vector<char>{INPUT.begin(), INPUT.end()}, std::as_bytes(DATA), 4_bytes);

    checkSkip<char>(std::as_bytes(DATA));
    checkSkip<std::array<char, 4>>(std::as_bytes(DATA));
    static_assert(!Skipable<std::optional<char>>);
    static_assert(!Skipable<std::unique_ptr<char>>);
    static_assert(!Skipable<std::vector<char>>);

    checkCopy<char>(std::as_bytes(DATA.subspan(0, 1)));
    checkCopy<std::array<char, 4>>(std::as_bytes(DATA));
    static_assert(!Copyable<std::optional<char>>);
    static_assert(!Copyable<std::unique_ptr<char>>);
    static_assert(!Copyable<std::vector<char>>);

    checkPrint('F', "F");
    checkPrint(INPUT, "4 [F, o, b, s, ]");
    checkPrint(std::optional<char>{}, "(none)");
    checkPrint(std::optional<char>{'F'}, "F");
    checkPrint(std::unique_ptr<char>{}, "(none)");
    checkPrint(std::make_unique<char>('F'), "F");
    checkPrint(std::vector<char>{}, "0 []");
    checkPrint(std::vector<char>{INPUT.begin(), INPUT.end()}, "4 [F, o, b, s, ]");

    checkSize('F', 1_bytes, true);
    checkSize(INPUT, 4 * 1_bytes, true);
    checkSize(std::optional<char>{}, 0_bytes, false);
    checkSize(std::optional<char>{'F'}, 1_bytes, false);
    checkSize(std::unique_ptr<char>{}, 0_bytes, false);
    checkSize(std::make_unique<char>('F'), 1_bytes, false);
    checkSize(std::vector<char>{}, 0_bytes, false);
    checkSize(std::vector<char>{INPUT.begin(), INPUT.end()}, 4 * 1_bytes, false);
  }

  void testIntegral() {
    const std::array<uint32_t, 4> DATA_BUFFER{0x42137011, 0x42000000, 0x17000017, 0x00450045};
    const std::span DATA{DATA_BUFFER};
    const std::array<uint32_t, 4> INPUT{0x11701342, 0x42, 0x17000017, 0x45004500};

    checkRead(0x11701342U, std::as_bytes(DATA));
    checkRead(INPUT, std::as_bytes(DATA));
    static_assert(!Readable<std::optional<uint32_t>>);
    static_assert(!Readable<std::unique_ptr<uint32_t>>);
    static_assert(!Readable<std::vector<uint32_t>>);

    checkWrite(0x11701342U, std::as_bytes(DATA.subspan(0, 1)));
    checkWrite(INPUT, std::as_bytes(DATA));
    checkWrite(std::optional<uint32_t>{}, {}, 0_bits);
    checkWrite(std::optional<uint32_t>{0x11701342U}, std::as_bytes(DATA.subspan(0, 1)), 4_bytes);
    checkWrite(std::unique_ptr<uint32_t>{}, {}, 0_bits);
    checkWrite(std::make_unique<uint32_t>(0x11701342U), std::as_bytes(DATA.subspan(0, 1)), 4_bytes);
    checkWrite(std::vector<uint32_t>{}, {}, 0_bits);
    checkWrite(std::vector<uint32_t>{INPUT.begin(), INPUT.end()}, std::as_bytes(DATA), 16_bytes);

    checkSkip<uint32_t>(std::as_bytes(DATA));
    checkSkip<std::array<uint32_t, 4>>(std::as_bytes(DATA));
    static_assert(!Skipable<std::optional<uint32_t>>);
    static_assert(!Skipable<std::unique_ptr<uint32_t>>);
    static_assert(!Skipable<std::vector<uint32_t>>);

    checkCopy<uint32_t>(std::as_bytes(DATA.subspan(0, 1)));
    checkCopy<std::array<uint32_t, 4>>(std::as_bytes(DATA));
    static_assert(!Copyable<std::optional<uint32_t>>);
    static_assert(!Copyable<std::unique_ptr<uint32_t>>);
    static_assert(!Copyable<std::vector<uint32_t>>);

    checkPrint(0x11701342U, "292557634");
    checkPrint(INPUT, "4 [292557634, 66, 385875991, 1157645568, ]");
    checkPrint(std::optional<uint32_t>{}, "(none)");
    checkPrint(std::optional<uint32_t>{0x11701342U}, "292557634");
    checkPrint(std::unique_ptr<uint32_t>{}, "(none)");
    checkPrint(std::make_unique<uint32_t>(0x11701342U), "292557634");
    checkPrint(std::vector<uint32_t>{}, "0 []");
    checkPrint(std::vector<uint32_t>{INPUT.begin(), INPUT.end()}, "4 [292557634, 66, 385875991, 1157645568, ]");

    checkSize(0x11701342U, 4_bytes, true);
    checkSize(INPUT, 4 * 4_bytes, true);
    checkSize(std::optional<uint32_t>{}, 0_bytes, false);
    checkSize(std::optional<uint32_t>{0x11701342U}, 4_bytes, false);
    checkSize(std::unique_ptr<uint32_t>{}, 0_bytes, false);
    checkSize(std::make_unique<uint32_t>(0x11701342U), 4_bytes, false);
    checkSize(std::vector<uint32_t>{}, 0_bytes, false);
    checkSize(std::vector<uint32_t>{INPUT.begin(), INPUT.end()}, 4 * 4_bytes, false);
  }

  void testEnum() {
    const std::array<Enum, 5> DATA_BUFFER{Enum::BAR, Enum::FOO, Enum::BAZ, Enum::FOO, Enum::BAR};
    const std::span DATA{DATA_BUFFER};

    checkRead(Enum::BAR, std::as_bytes(DATA.subspan(0, 1)));
    checkRead(DATA_BUFFER, std::as_bytes(DATA));
    static_assert(!Readable<std::optional<Enum>>);
    static_assert(!Readable<std::unique_ptr<Enum>>);
    static_assert(!Readable<std::vector<Enum>>);

    checkWrite(Enum::BAR, std::as_bytes(DATA.subspan(0, 1)));
    checkWrite(DATA_BUFFER, std::as_bytes(DATA));
    checkWrite(std::optional<Enum>{}, {}, 0_bytes);
    checkWrite(std::optional<Enum>{Enum::BAR}, std::as_bytes(DATA.subspan(0, 1)), 1_bytes);
    checkWrite(std::unique_ptr<Enum>{}, {}, 0_bytes);
    checkWrite(std::make_unique<Enum>(Enum::BAR), std::as_bytes(DATA.subspan(0, 1)), 1_bytes);
    checkWrite(std::vector<Enum>{}, {}, 0_bytes);
    checkWrite(std::vector<Enum>{DATA.begin(), DATA.end()}, std::as_bytes(DATA), 5_bytes);

    checkSkip<Enum>(std::as_bytes(DATA));
    checkSkip<std::array<Enum, 5>>(std::as_bytes(DATA));
    static_assert(!Skipable<std::optional<Enum>>);
    static_assert(!Skipable<std::unique_ptr<Enum>>);
    static_assert(!Skipable<std::vector<Enum>>);

    checkCopy<Enum>(std::as_bytes(DATA.subspan(0, 1)));
    checkCopy<std::array<Enum, 5>>(std::as_bytes(DATA));
    static_assert(!Copyable<std::optional<Enum>>);
    static_assert(!Copyable<std::unique_ptr<Enum>>);
    static_assert(!Copyable<std::vector<Enum>>);

    checkPrint(Enum::BAZ, "32");
    checkPrint(DATA_BUFFER, "5 [17, 1, 32, 1, 17, ]");
    checkPrint(std::optional<Enum>{}, "(none)");
    checkPrint(std::optional<Enum>{Enum::BAZ}, "32");
    checkPrint(std::unique_ptr<Enum>{}, "(none)");
    checkPrint(std::make_unique<Enum>(Enum::BAR), "17");
    checkPrint(std::vector<Enum>{}, "0 []");
    checkPrint(std::vector<Enum>{DATA.begin(), DATA.end()}, "5 [17, 1, 32, 1, 17, ]");

    checkSize(Enum::BAZ, 1_bytes, true);
    checkSize(DATA_BUFFER, 5_bytes, true);
    checkSize(std::optional<Enum>{}, 0_bytes, false);
    checkSize(std::optional<Enum>{Enum::FOO}, 1_bytes, false);
    checkSize(std::unique_ptr<Enum>{}, 0_bytes, false);
    checkSize(std::make_unique<Enum>(Enum::BAR), 1_bytes, false);
    checkSize(std::vector<Enum>{}, 0_bytes, false);
    checkSize(std::vector<Enum>{DATA.begin(), DATA.end()}, 5_bytes, false);
  }

  void testNonTrivial() {
    static_assert(!Readable<NonTrival>);
    static_assert(!Readable<std::array<NonTrival, 4>>);
    static_assert(!Readable<std::optional<NonTrival>>);
    static_assert(!Readable<std::unique_ptr<NonTrival>>);
    static_assert(!Readable<std::vector<NonTrival>>);

    static_assert(!Writeable<NonTrival>);
    static_assert(!Writeable<std::array<NonTrival, 4>>);
    static_assert(!Writeable<std::optional<NonTrival>>);
    static_assert(!Writeable<std::unique_ptr<NonTrival>>);
    static_assert(!Writeable<std::vector<NonTrival>>);

    static_assert(!Skipable<NonTrival>);
    static_assert(!Skipable<std::array<NonTrival, 4>>);
    static_assert(!Skipable<std::optional<NonTrival>>);
    static_assert(!Skipable<std::unique_ptr<NonTrival>>);
    static_assert(!Skipable<std::vector<NonTrival>>);

    static_assert(!Copyable<NonTrival>);
    static_assert(!Copyable<std::array<NonTrival, 4>>);
    static_assert(!Copyable<std::optional<NonTrival>>);
    static_assert(!Copyable<std::unique_ptr<NonTrival>>);
    static_assert(!Copyable<std::vector<NonTrival>>);

    static_assert(!Printable<NonTrival>);
    static_assert(!Printable<std::array<NonTrival, 4>>);
    static_assert(!Printable<std::optional<NonTrival>>);
    static_assert(!Printable<std::unique_ptr<NonTrival>>);
    static_assert(!Printable<std::vector<NonTrival>>);

    static_assert(!Sized<NonTrival>);
    static_assert(!Sized<std::array<NonTrival, 4>>);
    static_assert(!Sized<std::optional<NonTrival>>);
    static_assert(!Sized<std::unique_ptr<NonTrival>>);
    static_assert(!Sized<std::vector<NonTrival>>);
  }

  void testMemberFunctions() {
    const auto DATA = toBytes({// 1: 1, 300, -5
                               0x01, 0x01, 0x2C, 0xFB,
                               // 2: 17, 3, 17
                               0x11, 0x00, 0x03, 0x11,
                               // 3. 200, 30000, -100
                               0xC8, 0x75, 0x30, 0x9C});
    const std::array<Member, 3> VALUES{Member{1, 300, -5}, Member{17, 3, 17}, Member{200, 30000, -100}};

    checkRead(VALUES[0], DATA, 4_bytes);
    checkRead(VALUES, DATA, 3 * 4_bytes);
    static_assert(!Readable<std::optional<Member>>);
    static_assert(!Readable<std::unique_ptr<Member>>);
    static_assert(!Readable<std::vector<Member>>);

    checkWrite(VALUES[0], std::span{DATA}.subspan(0, 4), 4_bytes);
    checkWrite(VALUES, DATA, 3 * 4_bytes);
    checkWrite(std::optional<Member>{}, {}, 0_bytes);
    checkWrite(std::optional<Member>{VALUES[0]}, std::span{DATA}.subspan(0, 4), 4_bytes);
    checkWrite(std::unique_ptr<Member>{}, {}, 0_bytes);
    checkWrite(std::make_unique<Member>(VALUES[0]), std::span{DATA}.subspan(0, 4), 4_bytes);
    checkWrite(std::vector<Member>{}, {}, 0_bytes);
    checkWrite(std::vector<Member>(VALUES.begin(), VALUES.end()), DATA, 3 * 4_bytes);

    checkSkip<Member>(DATA, 4_bytes);
    checkSkip<std::array<Member, 3>>(DATA, 3 * 4_bytes);
    static_assert(!Skipable<std::optional<Member>>);
    static_assert(!Skipable<std::unique_ptr<Member>>);
    static_assert(!Skipable<std::vector<Member>>);

    checkCopy<Member>(std::span{DATA}.subspan(0, 4), 4_bytes);
    checkCopy<std::array<Member, 3>>(DATA, 3 * 4_bytes);
    static_assert(!Copyable<std::optional<Member>>);
    static_assert(!Copyable<std::unique_ptr<Member>>);
    static_assert(!Copyable<std::vector<Member>>);

    checkPrint(VALUES[0], "{1, 300, -5}");
    checkPrint(VALUES, "3 [{1, 300, -5}, {17, 3, 17}, {200, 30000, -100}, ]");
    checkPrint(std::optional<Member>{}, "(none)");
    checkPrint(std::optional<Member>{VALUES[0]}, "{1, 300, -5}");
    checkPrint(std::unique_ptr<Member>{}, "(none)");
    checkPrint(std::make_unique<Member>(VALUES[0]), "{1, 300, -5}");
    checkPrint(std::vector<Member>{}, "0 []");
    checkPrint(std::vector<Member>{VALUES.begin(), VALUES.end()},
               "3 [{1, 300, -5}, {17, 3, 17}, {200, 30000, -100}, ]");

    checkSize(VALUES[0], 4_bytes, false);
    checkSize(VALUES, 3 * 4_bytes, false);
    checkSize(std::optional<Member>{}, 0_bytes, false);
    checkSize(std::optional<Member>{VALUES[0]}, 4_bytes, false);
    checkSize(std::unique_ptr<Member>{}, 0_bytes, false);
    checkSize(std::make_unique<Member>(VALUES[0]), 4_bytes, false);
    checkSize(std::vector<Member>{}, 0_bytes, false);
    checkSize(std::vector<Member>{VALUES.begin(), VALUES.end()}, 3 * 4_bytes, false);
  }

  void testPOD() {
    const auto DATA = toBytes({// 1: -1, 300, 42
                               0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x01, 0x2C, 0x2A,
                               // 2: 17, 3, 65
                               0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x03, 0x41});
    const std::array<POD, 2> VALUES{POD{-1, 300, 42}, POD{17, 3, 65}};

    checkRead(VALUES[0], std::span{DATA}.subspan(0, 9), 9_bytes);
    checkRead(VALUES, DATA, 2 * 9_bytes);
    static_assert(!Readable<std::optional<POD>>);
    static_assert(!Readable<std::unique_ptr<POD>>);
    static_assert(!Readable<std::vector<POD>>);

    checkWrite(VALUES[0], std::span{DATA}.subspan(0, 9), 9_bytes);
    checkWrite(VALUES, DATA, 2 * 9_bytes);
    checkWrite(std::optional<POD>{}, {}, 0_bytes);
    checkWrite(std::optional<POD>{VALUES[0]}, std::span{DATA}.subspan(0, 9), 9_bytes);
    checkWrite(std::unique_ptr<POD>{}, {}, 0_bytes);
    checkWrite(std::make_unique<POD>(VALUES[0]), std::span{DATA}.subspan(0, 9), 9_bytes);
    checkWrite(std::vector<POD>{}, {}, 0_bytes);
    checkWrite(std::vector<POD>(VALUES.begin(), VALUES.end()), DATA, 2 * 9_bytes);

    checkSkip<POD>(std::span{DATA}.subspan(0, 9), 9_bytes);
    checkSkip<std::array<POD, 2>>(DATA, 2 * 9_bytes);
    static_assert(!Skipable<std::optional<POD>>);
    static_assert(!Skipable<std::unique_ptr<POD>>);
    static_assert(!Skipable<std::vector<POD>>);

    checkCopy<POD>(std::span{DATA}.subspan(0, 9), 9_bytes);
    checkCopy<std::array<POD, 2>>(DATA, 2 * 9_bytes);
    static_assert(!Copyable<std::optional<POD>>);
    static_assert(!Copyable<std::unique_ptr<POD>>);
    static_assert(!Copyable<std::vector<POD>>);

    checkPrint(VALUES[0], "POD{a = -1, foo = 300, c = *}");
    checkPrint(VALUES, "2 [POD{a = -1, foo = 300, c = *}, POD{a = 17, foo = 3, c = A}, ]");
    checkPrint(std::optional<POD>{}, "(none)");
    checkPrint(std::optional<POD>{VALUES[0]}, "POD{a = -1, foo = 300, c = *}");
    checkPrint(std::unique_ptr<POD>{}, "(none)");
    checkPrint(std::make_unique<POD>(VALUES[0]), "POD{a = -1, foo = 300, c = *}");
    checkPrint(std::vector<POD>{}, "0 []");
    checkPrint(std::vector<POD>{VALUES.begin(), VALUES.end()},
               "2 [POD{a = -1, foo = 300, c = *}, POD{a = 17, foo = 3, c = A}, ]");

    checkSize(VALUES[0], 9_bytes, true);
    checkSize(VALUES, 2 * 9_bytes, true);
    checkSize(std::optional<POD>{}, 0_bytes, false);
    checkSize(std::optional<POD>{VALUES[0]}, 9_bytes, false);
    checkSize(std::unique_ptr<POD>{}, 0_bytes, false);
    checkSize(std::make_unique<POD>(VALUES[0]), 9_bytes, false);
    checkSize(std::vector<POD>{}, 0_bytes, false);
    checkSize(std::vector<POD>{VALUES.begin(), VALUES.end()}, 2 * 9_bytes, false);
  }

  void testDynamicallySizedDestructurable() {
    const auto DATA = toBytes({// 1: 0xFF, "Foo"
                               0xFF, 0x03, 0x46, 0x6F, 0x6F,
                               // 2: 17, "Bars"
                               0x11, 0x04, 0x42, 0x61, 0x72, 0x73});
    const std::array<DynamicallySizedDestructurable, 2> VALUES = [] {
      std::array<DynamicallySizedDestructurable, 2> result{};
      result[0].u = 0xFF;
      result[0].s = "Foo";
      result[1].u = 0x11;
      result[1].s = "Bars";
      return result;
    }();

    checkRead(VALUES[0], std::span{DATA}.subspan(0, 5), 5_bytes);
    checkRead(VALUES, DATA, 5_bytes + 6_bytes);
    static_assert(!Readable<std::optional<DynamicallySizedDestructurable>>);
    static_assert(!Readable<std::unique_ptr<DynamicallySizedDestructurable>>);
    static_assert(!Readable<std::vector<DynamicallySizedDestructurable>>);

    checkWrite(VALUES[0], std::span{DATA}.subspan(0, 5), 5_bytes);
    checkWrite(VALUES, DATA, 5_bytes + 6_bytes);
    checkWrite(std::optional<DynamicallySizedDestructurable>{}, {}, 0_bytes);
    checkWrite(std::optional<DynamicallySizedDestructurable>{VALUES[0]}, std::span{DATA}.subspan(0, 5), 5_bytes);
    checkWrite(std::unique_ptr<DynamicallySizedDestructurable>{}, {}, 0_bytes);
    checkWrite(std::make_unique<DynamicallySizedDestructurable>(VALUES[0]), std::span{DATA}.subspan(0, 5), 5_bytes);
    checkWrite(std::vector<DynamicallySizedDestructurable>{}, {}, 0_bytes);
    checkWrite(std::vector<DynamicallySizedDestructurable>(VALUES.begin(), VALUES.end()), DATA, 5_bytes + 6_bytes);

    checkSkip<DynamicallySizedDestructurable>(std::span{DATA}.subspan(0, 5), 5_bytes);
    checkSkip<std::array<DynamicallySizedDestructurable, 2>>(DATA, 5_bytes + 6_bytes);
    static_assert(!Skipable<std::optional<DynamicallySizedDestructurable>>);
    static_assert(!Skipable<std::unique_ptr<DynamicallySizedDestructurable>>);
    static_assert(!Skipable<std::vector<DynamicallySizedDestructurable>>);

    checkCopy<DynamicallySizedDestructurable>(std::span{DATA}.subspan(0, 5), 5_bytes);
    checkCopy<std::array<DynamicallySizedDestructurable, 2>>(DATA, 5_bytes + 6_bytes);
    static_assert(!Copyable<std::optional<DynamicallySizedDestructurable>>);
    static_assert(!Copyable<std::unique_ptr<DynamicallySizedDestructurable>>);
    static_assert(!Copyable<std::vector<DynamicallySizedDestructurable>>);

    checkPrint(VALUES[0], "DynamicallySizedDestructurable{u = 255, s = \"Foo\"}");
    checkPrint(VALUES, "2 [DynamicallySizedDestructurable{u = 255, s = \"Foo\"}, DynamicallySizedDestructurable{u = "
                       "17, s = \"Bars\"}, ]");
    checkPrint(std::optional<DynamicallySizedDestructurable>{}, "(none)");
    checkPrint(std::optional<DynamicallySizedDestructurable>{VALUES[0]},
               "DynamicallySizedDestructurable{u = 255, s = \"Foo\"}");
    checkPrint(std::unique_ptr<DynamicallySizedDestructurable>{}, "(none)");
    checkPrint(std::make_unique<DynamicallySizedDestructurable>(VALUES[0]),
               "DynamicallySizedDestructurable{u = 255, s = \"Foo\"}");
    checkPrint(std::vector<DynamicallySizedDestructurable>{}, "0 []");
    checkPrint(std::vector<DynamicallySizedDestructurable>{VALUES.begin(), VALUES.end()},
               "2 [DynamicallySizedDestructurable{u = 255, s = \"Foo\"}, DynamicallySizedDestructurable{u = 17, s = "
               "\"Bars\"}, ]");

    checkSize(VALUES[0], 5_bytes, false);
    checkSize(VALUES, 5_bytes + 6_bytes, false);
    checkSize(std::optional<DynamicallySizedDestructurable>{}, 0_bytes, false);
    checkSize(std::optional<DynamicallySizedDestructurable>{VALUES[0]}, 5_bytes, false);
    checkSize(std::unique_ptr<DynamicallySizedDestructurable>{}, 0_bytes, false);
    checkSize(std::make_unique<DynamicallySizedDestructurable>(VALUES[0]), 5_bytes, false);
    checkSize(std::vector<DynamicallySizedDestructurable>{}, 0_bytes, false);
    checkSize(std::vector<DynamicallySizedDestructurable>{VALUES.begin(), VALUES.end()}, 5_bytes + 6_bytes, false);
  }

  void testTuple() {
    using TrivialTuple = std::tuple<uint32_t, Enum>;
    using PodTuple = std::tuple<POD, uint32_t, Enum>;
    using MemberTuple = std::tuple<Member, uint32_t, Enum>;

    const auto TRIVIAL_DATA = toBytes({0x00, 0x01, 0xB9, 0xD9, 0x20});
    const auto POD_DATA = toBytes({0xFF, 0xFF, 0xFF, 0xF3, 0x00, 0x12, 0xD6, 0x87, 0x34, 0x00, 0x01, 0xB5, 0xF1, 0x11});
    const auto MEMBER_DATA = toBytes({0x11, 0x01, 0x90, 0xFD, 0x00, 0x0F, 0x12, 0x06, 0x01});

    const TrivialTuple TRIVIAL{113113, Enum::BAZ};
    const PodTuple PODV{{-13, 1234567, '4'}, 112113, Enum::BAR};
    const MemberTuple MEMBER{{17, 400, -3}, 987654, Enum::FOO};

    checkRead(TRIVIAL, TRIVIAL_DATA, 5_bytes);
    checkRead(PODV, POD_DATA, 14_bytes);
    checkRead(MEMBER, MEMBER_DATA, 9_bytes);

    checkWrite(TRIVIAL, TRIVIAL_DATA, 5_bytes);
    checkWrite(PODV, POD_DATA, 14_bytes);
    checkWrite(MEMBER, MEMBER_DATA, 9_bytes);

    checkSkip<TrivialTuple>(TRIVIAL_DATA, 5_bytes);
    checkSkip<PodTuple>(POD_DATA, 14_bytes);
    checkSkip<MemberTuple>(MEMBER_DATA, 9_bytes);

    checkCopy<TrivialTuple>(TRIVIAL_DATA, 5_bytes);
    checkCopy<PodTuple>(POD_DATA, 14_bytes);
    checkCopy<MemberTuple>(MEMBER_DATA, 9_bytes);

    checkPrint(TRIVIAL, "(113113, 32, )");
    checkPrint(PODV, "(POD{a = -13, foo = 1234567, c = 4}, 112113, 17, )");
    checkPrint(MEMBER, "({17, 400, -3}, 987654, 1, )");

    checkSize(TRIVIAL, 5_bytes, true);
    checkSize(PODV, 14_bytes, true);
    checkSize(MEMBER, 9_bytes, false);
  }

  void testVariant() {
    using TrivialVariant = std::variant<uint32_t, Enum>;
    using TrivialLargeVariant = std::variant<uint32_t, LargeEnum>;
    using PodVariant = std::variant<POD, uint32_t, Enum>;
    using MemberVariant = std::variant<Member, uint32_t, Enum>;

    const std::vector<std::byte> TRIVIAL_DATA{std::byte{17}};
    const auto LARGE_DATA = toBytes({0x00, 0x01, 0xC9, 0x7E});
    const auto POD_DATA = toBytes({0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x0E, 0x37});
    const auto MEMBER_DATA = toBytes({0x11, 0x00, 0x16, 0x2D});

    const TrivialVariant TRIVIAL{Enum::BAR};
    const TrivialLargeVariant LARGE{117118U};
    const PodVariant PODV{POD{13, 14, '7'}};
    const MemberVariant MEMBER{Member{17, 22, 45}};

    static_assert(!Readable<TrivialVariant>);
    static_assert(!Readable<TrivialLargeVariant>);
    static_assert(!Readable<PodVariant>);
    static_assert(!Readable<MemberVariant>);

    checkWrite(TRIVIAL, TRIVIAL_DATA, 1_bytes);
    checkWrite(LARGE, LARGE_DATA, 4_bytes);
    checkWrite(PODV, POD_DATA, 9_bytes);
    checkWrite(MEMBER, MEMBER_DATA, 4_bytes);

    static_assert(!Skipable<TrivialVariant>);
    // skipable without knowing the active member, since all members are equal sized
    checkSkip<TrivialLargeVariant>(LARGE_DATA, 4_bytes);
    static_assert(!Skipable<PodVariant>);
    static_assert(!Skipable<MemberVariant>);

    static_assert(!Copyable<TrivialVariant>);
    // copyable without knowing the active member, since all members are equal sized
    checkCopy<TrivialLargeVariant>(LARGE_DATA, 4_bytes);
    static_assert(!Copyable<PodVariant>);
    static_assert(!Copyable<MemberVariant>);

    checkPrint(TRIVIAL, "17");
    checkPrint(LARGE, "117118");
    checkPrint(PODV, "POD{a = 13, foo = 14, c = 7}");
    checkPrint(MEMBER, "{17, 22, 45}");

    checkSize(TRIVIAL, 1_bytes, false);
    checkSize(LARGE, 4_bytes, true);
    checkSize(PODV, 9_bytes, false);
    checkSize(MEMBER, 4_bytes, false);
  }

  void testRawBytes() {
    std::array<std::byte, 4> DATA{std::byte{0x14}, std::byte{0x04}, std::byte{0x10}, std::byte{0x11}};

    checkRead(DATA[0], std::span{DATA}.subspan(0, 1));
    checkRead(DATA, DATA);
    static_assert(!Readable<std::vector<std::byte>>);

    checkWrite(DATA[0], std::span{DATA}.subspan(0, 1));
    checkWrite(DATA, DATA);
    checkWrite(std::vector<std::byte>(DATA.begin(), DATA.end()), DATA, 4_bytes);

    checkSkip<std::byte>(std::span{DATA}.subspan(0, 1));
    checkSkip<std::array<std::byte, 4>>(DATA);
    static_assert(!Skipable<std::vector<std::byte>>);

    checkCopy<std::byte>(std::span{DATA}.subspan(0, 1));
    checkCopy<std::array<std::byte, 4>>(DATA);
    static_assert(!Copyable<std::vector<std::byte>>);

    checkPrint(DATA[0], "0x14");
    checkPrint(DATA, "4 [0x14, 0x04, 0x10, 0x11, ]");
    checkPrint(std::vector<std::byte>(DATA.begin(), DATA.end()), "4 [0x14, 0x04, 0x10, 0x11, ]");

    checkSize(DATA[0], 1_bytes, true);
    checkSize(DATA, 4_bytes, true);
    checkSize(std::vector<std::byte>(DATA.begin(), DATA.end()), 4_bytes, false);
  }

  void testSizes() {
    static_assert(minNumBits<std::byte>() == 8_bits);
    static_assert(maxNumBits<std::byte>() == 8_bits);
    static_assert(numBits(std::byte{7}) == 8_bits);

    static_assert(minNumBits<POD>() == 9_bytes);
    static_assert(maxNumBits<POD>() == 9_bytes);
    static_assert(numBits(POD{7, 3, 1}) == 9_bytes);

    static_assert(minNumBits<std::array<std::byte, 3>>() == 24_bits);
    static_assert(maxNumBits<std::array<std::byte, 3>>() == 24_bits);
    static_assert(numBits(std::array<std::byte, 3>{}) == 24_bits);

    static_assert(minNumBits<std::optional<std::byte>>() == 0_bits);
    static_assert(maxNumBits<std::optional<std::byte>>() == 8_bits);
    static_assert(numBits(std::optional<std::byte>{}) == 0_bits);
    static_assert(numBits(std::optional<std::byte>{std::byte{0x10}}) == 8_bits);

    static_assert(minNumBits<std::unique_ptr<std::byte>>() == 0_bits);
    static_assert(maxNumBits<std::unique_ptr<std::byte>>() == 8_bits);
    TEST_ASSERT_EQUALS(0_bits, numBits(std::unique_ptr<std::byte>{}));
    TEST_ASSERT_EQUALS(1_bytes, numBits(std::make_unique<std::byte>(std::byte{0x11})));

    static_assert(minNumBits<std::vector<std::byte>>() == 0_bits);
    static_assert(maxNumBits<std::vector<std::byte>>() != 24_bits);
    TEST_ASSERT_EQUALS(0_bits, numBits(std::vector<std::byte>{}));
    TEST_ASSERT_EQUALS(2_bytes, numBits(std::vector<std::byte>{std::byte{0x10}, std::byte{0x11}}));

    static_assert(minNumBits<std::variant<std::byte, POD>>() == 8_bits);
    static_assert(maxNumBits<std::variant<std::byte, POD>>() == 9_bytes);
    static_assert(numBits(std::variant<std::byte, POD>{std::byte{5}}) == 8_bits);
    static_assert(numBits(std::variant<std::byte, POD>{POD{5, 4, 3}}) == 9_bytes);
  }

  void testReadOptional() {
    std::array<std::byte, 4> DATA{std::byte{0x14}, std::byte{0x04}, std::byte{0x10}, std::byte{0x11}};
    BitReader reader{DATA};
    TEST_ASSERT_FALSE(readOptional<uint32_t>(reader, false));
    TEST_ASSERT_EQUALS(0_bits, reader.position());
    TEST_ASSERT_EQUALS(0x14041011U, readOptional<uint32_t>(reader, true));
    TEST_ASSERT_EQUALS(4_bytes, reader.position());
  }
};

class TestTypes : public TestBase {
public:
  TestTypes() : TestBase("Types") {
    TEST_ADD(TestTypes::testBit);
    TEST_ADD(TestTypes::testBits);
    TEST_ADD(TestTypes::testBytes);
    TEST_ADD(TestTypes::testByte);
    TEST_ADD(TestTypes::testFixedBits);
    TEST_ADD(TestTypes::testFixedBytes);
    TEST_ADD(TestTypes::testFixedByte);
    TEST_ADD(TestTypes::testChar);
    TEST_ADD(TestTypes::testExpGolombBits);
    TEST_ADD(TestTypes::testSignedExpGolombBits);
    TEST_ADD(TestTypes::testFibonacciBits);
    TEST_ADD(TestTypes::testNegaFibonacciBits);
    TEST_ADD(TestTypes::testOptionalBits);
    TEST_ADD(TestTypes::testBitList);
    TEST_ADD(TestTypes::testChars);
    TEST_ADD(TestTypes::testPrefixString);
    TEST_ADD(TestTypes::testSizedList);
    TEST_ADD(TestTypes::testPaddedValue);
    TEST_ADD(TestTypes::testFixedSizeVariant);
    TEST_ADD(TestTypes::testAlignmentBits);
  }

  void testBit() { checkValueBitfield<Bit, bool>(true, toBytes({0x80}), "0b1", 1_bits, true); }

  void testBits() {
    checkValueBitfield<Bits<5>, uint8_t>(0, toBytes({0x00}), "0b00000", 5_bits, true);
    checkValueBitfield<Bits<3>, uint8_t>(2, toBytes({0x40}), "0b010", 3_bits, true);
    checkValueBitfield<Bits<13>, uint16_t>(8191, toBytes({0xFF, 0xF8}), "0x1fff", 13_bits, true);
    checkValueBitfield<Bits<8, Enum>>(Enum::FOO, toBytes({0x01}), "0x01", 8_bits, true);
  }

  void testBytes() {
    checkValueBitfield<Bytes<3>, uint32_t>(123456U, toBytes({0x01, 0xE2, 0x40}), "0x01e240", 3_bytes, true);
    checkValueBitfield<Bytes<7>, uint64_t>(0x91A2B3C4855E6FULL, toBytes({0x91, 0xA2, 0xB3, 0xC4, 0x85, 0x5E, 0x6F}),
                                           "0x91a2b3c4855e6f", 7_bytes, true);
  }

  void testByte() {
    checkValueBitfield<Byte<>>(uint8_t{17}, toBytes({0x11}), "0x11", 1_bytes, true);
    checkValueBitfield<Byte<Enum>>(Enum::BAR, toBytes({0x11}), "0x11", 1_bytes, true);
  }

  void testFixedBits() {
    checkBitfield(FixedBits<17, 0x123>{}, toBytes({0x00, 0x91, 0x80}), "0x00123", 17_bits, true);
    TEST_ASSERT((FixedBits<17, 0x123>{}.matches(0x123)));
    TEST_ASSERT(!(FixedBits<17, 0x123>{}.matches(0x12)));
  }

  void testFixedBytes() {
    checkBitfield(FixedBytes<2, 0xABCD>{}, toBytes({0xAB, 0xCD}), "0xabcd", 2_bytes, true);
    TEST_ASSERT((FixedBytes<2, 0xABCD>{}.matches(0xABCD)));
    TEST_ASSERT(!(FixedBytes<2, 0xABCD>{}.matches(0xAB)));
  }

  void testFixedByte() {
    checkBitfield(FixedByte<0x17>{}, toBytes({0x17}), "0x17", 1_bytes, true);
    checkBitfield(FixedByte<Enum::BAR>{}, toBytes({0x11}), "0x11", 1_bytes, true);
  }

  void testChar() { checkValueBitfield<Char>('7', toBytes({0x37}), "7", 1_bytes, true); }

  void testExpGolombBits() {
    static_assert(sizeof(ExpGolombBits<>::value_type) >= sizeof(uint64_t));
    static_assert(ExpGolombBits<uint64_t>::minNumBits() == 1_bits);
    static_assert(ExpGolombBits<uint64_t>::maxNumBits() == 129_bits);

    checkValueBitfield<ExpGolombBits<>>(uintmax_t{0}, toBytes({0x80}), "0", 1_bits, false);
    checkValueBitfield<ExpGolombBits<>>(uintmax_t{107}, toBytes({0x03, 0x60}), "107", 13_bits, false);
    checkValueBitfield<ExpGolombBits<>>(
        uintmax_t{0xFFFFFFFFFFFFFFFE},
        toBytes({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE}),
        "18446744073709551614", 127_bits, false);
  }

  void testSignedExpGolombBits() {
    static_assert(sizeof(SignedExpGolombBits<>::value_type) >= sizeof(int64_t));
    static_assert(SignedExpGolombBits<int16_t>::minNumBits() == 1_bits);
    static_assert(SignedExpGolombBits<int16_t>::maxNumBits() == 33_bits);

    checkValueBitfield<SignedExpGolombBits<>>(intmax_t{0}, toBytes({0x80}), "0", 1_bits, false);
    checkValueBitfield<SignedExpGolombBits<>>(intmax_t{107}, toBytes({0x01, 0xAC}), "107", 15_bits, false);
    checkValueBitfield<SignedExpGolombBits<>>(intmax_t{-107}, toBytes({0x01, 0xAE}), "-107", 15_bits, false);
    checkValueBitfield<SignedExpGolombBits<>>(
        intmax_t{0x3FFFFFFFFFFFFFFF},
        toBytes({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0}),
        "4611686018427387903", 125_bits, false);
    checkValueBitfield<SignedExpGolombBits<>>(
        intmax_t{-0x3FFFFFFFFFFFFFFF},
        toBytes({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8}),
        "-4611686018427387903", 125_bits, false);
  }

  void testFibonacciBits() {
    static_assert(sizeof(FibonacciBits<>::value_type) >= sizeof(uint32_t));
    static_assert(FibonacciBits<uint32_t>::minNumBits() == 2_bits);
    static_assert(FibonacciBits<uint32_t>::maxNumBits() == 48_bits);

    checkValueBitfield<FibonacciBits<>>(uint32_t{1}, toBytes({0xC0}), "1", 2_bits, false);
    checkValueBitfield<FibonacciBits<>>(uint32_t{107}, toBytes({0x14, 0x60}), "107", 11_bits, false);
    checkValueBitfield<FibonacciBits<>>(uint32_t{0xFFFFFFFF}, toBytes({0x24, 0x88, 0x08, 0xA2, 0xA1, 0x16}),
                                        "4294967295", 47_bits, false);
  }

  void testNegaFibonacciBits() {
    static_assert(sizeof(NegaFibonacciBits<>::value_type) >= sizeof(int32_t));
    static_assert(NegaFibonacciBits<int32_t>::minNumBits() == 2_bits);
    static_assert(NegaFibonacciBits<int32_t>::maxNumBits() == 48_bits);

    checkValueBitfield<NegaFibonacciBits<>>(int32_t{1}, toBytes({0xC0}), "1", 2_bits, false);
    checkValueBitfield<NegaFibonacciBits<>>(int32_t{-1}, toBytes({0x60}), "-1", 3_bits, false);
    checkValueBitfield<NegaFibonacciBits<>>(int32_t{107}, toBytes({0x0A, 0x30}), "107", 12_bits, false);
    checkValueBitfield<NegaFibonacciBits<>>(int32_t{-107}, toBytes({0xA0, 0x98}), "-107", 13_bits, false);
    checkValueBitfield<NegaFibonacciBits<>>(int32_t{0x3FFFFFFF}, toBytes({0x8A, 0x81, 0x02, 0x29, 0x54, 0x0C}),
                                            "1073741823", 46_bits, false);
    checkValueBitfield<NegaFibonacciBits<>>(int32_t{-0x3FFFFFFF}, toBytes({0x20, 0x29, 0x52, 0x81, 0x01, 0x58}),
                                            "-1073741823", 45_bits, false);
  }

  void testOptionalBits() {
    static_assert(OptionalBits<int16_t>::minNumBits() == 1_bits);
    static_assert(OptionalBits<int16_t>::maxNumBits() == 17_bits);

    checkValueBitfield<OptionalBits<uint8_t>>(std::optional<uint8_t>{}, toBytes({0x00}), "(empty)", 1_bits, false);
    checkValueBitfield<OptionalBits<uint8_t, false>>(std::optional<uint8_t>{}, toBytes({0x80}), "(empty)", 1_bits,
                                                     false);
    checkValueBitfield<OptionalBits<uint8_t>>(std::optional<uint8_t>{17}, toBytes({0x88, 0x80}), "17", 9_bits, false);

    checkValueBitfield<OptionalBits<Bits<3>>>(std::optional<Bits<3>>{}, toBytes({0x00}), "(empty)", 1_bits, false);
    checkValueBitfield<OptionalBits<Bits<3>, false>>(std::optional<Bits<3>>{}, toBytes({0x80}), "(empty)", 1_bits,
                                                     false);
    checkValueBitfield<OptionalBits<Bits<3>>>(std::optional<Bits<3>>{Bits<3>{}}, toBytes({0x80}), "0b000", 4_bits,
                                              false);

    checkValueBitfield<OptionalBits<uint64_t, false>>(std::optional<uint64_t>{0xFFFFFFFFFFFFFFFFULL},
                                                      toBytes({0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x80}),
                                                      "18446744073709551615", 65_bits, false);
  }

  void testBitList() {
    static_assert(BitList<7, uint16_t>::minNumBits() == 7_bits);
    static_assert(BitList<7, uint16_t>::maxNumBits() == 7_bits + sizeof(uint16_t) * 128_bytes);

    checkValueBitfield<BitList<3, uint8_t>>(std::vector<uint8_t>{}, toBytes({0x00}), "0 []", 3_bits, false);
    checkValueBitfield<BitList<3, uint8_t>>(std::vector<uint8_t>{0x01, 0x02, 0x03}, toBytes({0x60, 0x20, 0x40, 0x60}),
                                            "3 [1, 2, 3, ]", 27_bits, false);

    Bytes<3> val{};
    val.set(4000);
    checkValueBitfield<BitList<8, Bytes<3>>>(std::vector<Bytes<3>>{val, val},
                                             toBytes({0x02, 0x00, 0x0F, 0xA0, 0x00, 0x0F, 0xA0}),
                                             "2 [0x000fa0, 0x000fa0, ]", 7_bytes, false);
  }

  void testChars() {
    checkValueBitfield<Chars<5>>(std::array<char, 5>{'F', 'o', 'o', ' ', '!'}, toBytes({'F', 'o', 'o', ' ', '!'}),
                                 "\"Foo !\"", 5_bytes, true);
  }

  void testPrefixString() {
    static_assert(PrefixString<7>::minNumBits() == 7_bits);
    static_assert(PrefixString<7>::maxNumBits() == 7_bits + 128_bytes);

    checkValueBitfield<PrefixString<8>>(std::string{}, toBytes({0x00}), "\"\"", 1_bytes, false);
    checkValueBitfield<PrefixString<8>>(std::string{"Foo Bar"}, toBytes({0x07, 'F', 'o', 'o', ' ', 'B', 'a', 'r'}),
                                        "\"Foo Bar\"", 8_bytes, false);
  }

  void testSizedList() {
    static_assert(SizedList<7, uint16_t>::minNumBits() == 7_bits);
    static_assert(SizedList<7, uint16_t>::maxNumBits() == 7_bits + 128_bytes);

    checkValueBitfield<SizedList<8, uint64_t>>(std::vector<uint64_t>{}, toBytes({0x00}), "0 []", 1_bytes, false);
    checkValueBitfield<SizedList<3, uint8_t>>(std::vector<uint8_t>{1, 42, 17}, toBytes({0x60, 0x25, 0x42, 0x20}),
                                              "3 [1, 42, 17, ]", 27_bits, false);

    Bytes<3> val{};
    val.set(4000);
    checkValueBitfield<SizedList<8, Bytes<3>>>(std::vector<Bytes<3>>{val, val},
                                               toBytes({0x06, 0x00, 0x0F, 0xA0, 0x00, 0x0F, 0xA0}),
                                               "2 [0x000fa0, 0x000fa0, ]", 7_bytes, false);
  }

  void testPaddedValue() {
    checkValueBitfield<PaddedValue<uint8_t, 24, uint8_t, 0xDE>>(uint8_t{0x17}, toBytes({0x17, 0xDE, 0xDE}), "23",
                                                                24_bits, true);
    checkValueBitfield<PaddedValue<Bit, 7, bool, true>>(Bit{}, toBytes({0x7E}), "0b0", 7_bits, true);

    Bits<3> val{};
    val.set(5);
    BitList<3, Bits<3>> list{};
    list.set({val, {}, val});
    checkValueBitfield<PaddedValue<BitList<3, Bits<3>>, 17, bool, true>>(list, toBytes({0x74, 0x5F, 0x80}),
                                                                         "3 [0b101, 0b000, 0b101, ]", 17_bits, true);
  }

  void testFixedSizeVariant() {}

  void testAlignmentBits() {
    static_assert(AlignmentBits<8, false>::minNumBits() == 0_bits);
    static_assert(AlignmentBits<8, false>::maxNumBits() == 7_bits);

    const AlignmentBits<7, true> value{};
    const std::vector<uint8_t> data{};
    checkRead(value, std::as_bytes(std::span{data}), 0_bits);
    checkWrite(value, std::as_bytes(std::span{data}), 0_bits);
    checkPrint(value, "(fill to 7 bits with 1)");
    static_assert(!Sized<AlignmentBits<7, true>>);
    checkSkip<AlignmentBits<7, true>>(std::as_bytes(std::span{data}), 0_bits);
    checkCopy<AlignmentBits<7, true>>(std::as_bytes(std::span{data}), 0_bits);
  }

private:
  template <typename T>
  void checkBitfield(const T &value, std::span<const std::byte> data, std::string_view expectedString, BitCount numBits,
                     bool fixedSize)
    requires(BitField<T>)
  {
    checkRead(value, data, numBits);
    checkWrite(value, data, numBits);
    checkPrint(value, expectedString);
    checkSize(value, numBits, fixedSize);
    checkSkip<T>(data, numBits);
    checkCopy<T>(data, numBits);
  }

  template <typename T, typename U>
  void checkValueBitfield(const U &value, std::span<const std::byte> data, std::string_view expectedString,
                          BitCount numBits, bool fixedSize)
    requires(ValueBitField<T, U> && std::is_same_v<typename T::value_type, U>)
  {
    T field{};
    field = value;
    checkBitfield(field, data, expectedString, numBits, fixedSize);
    TEST_ASSERT_EQUALS(value, static_cast<T>(field));

    field = T{};
    field.set(value);
    checkBitfield(field, data, expectedString, numBits, fixedSize);
    TEST_ASSERT_EQUALS(value, field.get());
  }

  template <typename T, typename U>
  void checkWriteOnlyField(const U &value, std::span<const std::byte> data, std::string_view expectedString,
                           BitCount numBits, bool fixedSize) {
    T field{};
    field = value;
    static_assert(!Readable<T>);
    checkWrite(field, data, numBits);
    checkPrint(field, expectedString);
    checkSize(field, numBits, fixedSize);
    checkSkip<T>(data, numBits);
    checkCopy<T>(data, numBits);
    TEST_ASSERT_EQUALS(value, static_cast<T>(field));

    field = T{};
    field.set(value);
    static_assert(!Readable<T>);
    checkWrite(field, data, numBits);
    checkPrint(field, expectedString);
    checkSize(field, numBits, fixedSize);
    checkSkip<T>(data, numBits);
    checkCopy<T>(data, numBits);
    TEST_ASSERT_EQUALS(value, field.get());
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
    TEST_ADD(TestCache::testCreateCache);
    TEST_ADD(TestCache::testFillCache);
    TEST_ADD(TestCache::testReadCache);
    TEST_ADD(TestCache::testFlushCache);
    TEST_ADD(TestCache::testWriteCache);
    TEST_ADD(TestCache::testWriteAndFlushCache);
  }

  void testCreateCache() {
    TEST_ASSERT_EQUALS((Cache{0x1234000000000000, 16_bits}), createCache(0x1234, 16_bits));
    static_assert(createCache(0x7, 3_bits).value == 0xE000000000000000);
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

  void testWriteCache() {
    TEST_ASSERT_EQUALS((Cache{0x091A000000000000, 17_bits}), writeCacheHelper(Cache{}, 0x1234, 17_bits));
    TEST_ASSERT_EQUALS((Cache{0x4000000000000000, 3_bits}), writeCacheHelper(Cache{}, 0x2, 3_bits));
    TEST_ASSERT_EQUALS((Cache{0x1234091A00000000, 33_bits}),
                       writeCacheHelper(Cache{0x1234000000000000, 16_bits}, 0x1234, 17_bits));
    TEST_ASSERT_EQUALS((Cache{0xE800000000000000, 6_bits}),
                       writeCacheHelper(Cache{0xE000000000000000, 3_bits}, 0x2, 3_bits));
    TEST_ASSERT_EQUALS((Cache{0x1224680000000000, 23_bits}),
                       writeCacheHelper(Cache{0x1200000000000000, 7_bits}, 0x1234, 16_bits));
  }

  void testWriteAndFlushCache() {
    TEST_ASSERT_EQUALS(0x091A000000000000U, writeAndFlushCacheHelper(Cache{}, 0x1234, 17_bits).first);
    TEST_ASSERT_EQUALS((Cache{0x0, 1_bits}), writeAndFlushCacheHelper(Cache{}, 0x1234, 17_bits).second);
    TEST_ASSERT_EQUALS(0x0U, writeAndFlushCacheHelper(Cache{}, 0x2, 3_bits).first);
    TEST_ASSERT_EQUALS((Cache{0x4000000000000000, 3_bits}), writeAndFlushCacheHelper(Cache{}, 0x2, 3_bits).second);
    TEST_ASSERT_EQUALS(0x1234091A00000000U,
                       writeAndFlushCacheHelper(Cache{0x1234000000000000, 16_bits}, 0x1234, 17_bits).first);
    TEST_ASSERT_EQUALS((Cache{0x0, 1_bits}),
                       writeAndFlushCacheHelper(Cache{0x1234000000000000, 16_bits}, 0x1234, 17_bits).second);
    TEST_ASSERT_EQUALS(0x0U, writeAndFlushCacheHelper(Cache{0xE000000000000000, 3_bits}, 0x2, 3_bits).first);
    TEST_ASSERT_EQUALS((Cache{0xE800000000000000, 6_bits}),
                       writeAndFlushCacheHelper(Cache{0xE000000000000000, 3_bits}, 0x2, 3_bits).second);
    TEST_ASSERT_EQUALS(0x1224000000000000U,
                       writeAndFlushCacheHelper(Cache{0x1200000000000000, 7_bits}, 0x1234, 16_bits).first);
    TEST_ASSERT_EQUALS((Cache{0x6800000000000000, 7_bits}),
                       writeAndFlushCacheHelper(Cache{0x1200000000000000, 7_bits}, 0x1234, 16_bits).second);
    TEST_ASSERT_EQUALS(0x123456789ABCDEC4U,
                       writeAndFlushCacheHelper(Cache{0x123456789ABCDE80, 57_bits}, 0x112, 9_bits).first);
    TEST_ASSERT_EQUALS((Cache{0x8000000000000000, 2_bits}),
                       writeAndFlushCacheHelper(Cache{0x123456789ABCDE80, 57_bits}, 0x112, 9_bits).second);
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
    if (cacheSize != (cache.size + numBits))
      throw;
    return std::make_pair(result, cache);
  }

  static std::pair<std::uintmax_t, Cache> flushFullCacheBytesHelper(Cache cache) {
    Cache output{};
    flushFullCacheBytes(
        cache,
        [&output](std::byte nextByte) {
          if (output.size >= sizeof(std::uintmax_t) * 1_bytes)
            return false;
          std::uintmax_t tmp = static_cast<uint8_t>(nextByte);
          output.value |= tmp << (sizeof(std::uintmax_t) * 1_bytes - 1_bytes - output.size);
          output.size += 1_bytes;
          return true;
        },
        []() { throw; });
    return std::make_pair(output.value, cache);
  }

  static Cache writeCacheHelper(Cache cache, std::uintmax_t value, BitCount numBits) {
    writeToCache(cache, value, numBits);
    return cache;
  }

  static std::pair<std::uintmax_t, Cache> writeAndFlushCacheHelper(Cache cache, std::uintmax_t value,
                                                                   BitCount numBits) {
    Cache output{};
    writeToCacheAndFlushFullBytes(
        cache, value, numBits,
        [&output](std::byte nextByte) {
          if (output.size >= sizeof(std::uintmax_t) * 1_bytes)
            return false;
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

    TEST_ASSERT_EQUALS("17b", (17_bits).toString());
    TEST_ASSERT_EQUALS("2kb", (2048_bits).toString());
    TEST_ASSERT_EQUALS("2.1kb", (2148_bits).toString());
    TEST_ASSERT_EQUALS("325B", (325_bytes).toString());
    TEST_ASSERT_EQUALS("325MB", (340787200_bytes).toString());
    TEST_ASSERT_EQUALS("334.54MB", (350787200_bytes).toString());
  }
};

int main(int argc, char **argv) {
  Test::registerSuite(Test::newInstance<TestHelper>, "helper");
  Test::registerSuite(Test::newInstance<TestIO>, "io");
  Test::registerSuite(Test::newInstance<TestTypes>, "types");
  Test::registerSuite(Test::newInstance<TestCache>, "cache");
  Test::registerSuite(Test::newInstance<TestSizes>, "sizes");
  return Test::runSuites(argc, argv);
}
