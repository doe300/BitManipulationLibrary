#include "types.hpp"

#include "errors.hpp"
#include "test_helper.hpp"

#include "cpptest-main.h"

using namespace bml;

struct TestBase : public Test::Suite {
protected:
  TestBase(const std::string &name) : Test::Suite(name) {}

  template <typename T>
  void checkRead(const T &expectedValue, std::span<const std::byte> data, BitCount numBits = ByteCount{sizeof(T)})
    requires(Readable<T>)
  {
    BitReader reader{data};
    TEST_ASSERT(numBits == 0_bits || reader.hasMoreBytes());
    T value{};
    TEST_THROWS_NOTHING(bml::read(reader, value));
    TEST_ASSERT_EQUALS(numBits, reader.position());
    TEST_ASSERT_EQUALS(expectedValue, value);
    TEST_ASSERT(numBits != ByteCount{data.size()} || !reader.hasMoreBytes());

    reader = createGeneratorReader(data);
    TEST_ASSERT(numBits == 0_bits || reader.hasMoreBytes());
    value = T{};
    TEST_THROWS_NOTHING(bml::read(reader, value));
    TEST_ASSERT_EQUALS(numBits, reader.position());
    TEST_ASSERT_EQUALS(expectedValue, value);
    TEST_ASSERT(numBits != ByteCount{data.size()} || !reader.hasMoreBytes());
  }

  template <typename T>
  void checkWrite(const T &value, std::span<const std::byte> expectedData, BitCount numBits = ByteCount{sizeof(T)})
    requires(Writeable<T>)
  {
    std::vector<std::byte> data(expectedData.size());
    BitWriter writer{data};
    TEST_THROWS_NOTHING(bml::write(writer, value));
    TEST_ASSERT_EQUALS(numBits, writer.position());
    writer.flush();
    TEST_ASSERT_EQUALS(expectedData, data);

    data = std::vector<std::byte>(expectedData.size());
    writer = createConsumerWriter(data);
    TEST_THROWS_NOTHING(bml::write(writer, value));
    TEST_ASSERT_EQUALS(numBits, writer.position());
    writer.flush();
    TEST_ASSERT_EQUALS(expectedData, data);
  }

  template <typename T>
  void checkSkip(std::span<const std::byte> data, BitCount numBits = ByteCount{sizeof(T)})
    requires(Skipable<T>)
  {
    BitReader reader{data};
    TEST_THROWS_NOTHING(bml::skip<T>(reader));
    TEST_ASSERT_EQUALS(numBits, reader.position());

    reader = createGeneratorReader(data);
    TEST_THROWS_NOTHING(bml::skip<T>(reader));
    TEST_ASSERT_EQUALS(numBits, reader.position());
  }

  template <typename T>
  void checkCopy(std::span<const std::byte> expectedData, BitCount numBits = ByteCount{sizeof(T)})
    requires(Copyable<T>)
  {
    {
      // Copy directly on byte buffer
      BitReader reader{expectedData};
      std::vector<std::byte> data(expectedData.size());
      BitWriter writer{data};
      TEST_THROWS_NOTHING(bml::copy<T>(reader, writer));
      TEST_ASSERT_EQUALS(numBits, reader.position());
      TEST_ASSERT_EQUALS(numBits, writer.position());
      writer.flush();
      TEST_ASSERT_EQUALS(expectedData, data);
    }

    {
      // Copy on consumers/generators
      BitReader reader{createGeneratorReader(expectedData)};
      std::vector<std::byte> data(expectedData.size());
      BitWriter writer{createConsumerWriter(data)};
      TEST_THROWS_NOTHING(bml::copy<T>(reader, writer));
      TEST_ASSERT_EQUALS(numBits, reader.position());
      TEST_ASSERT_EQUALS(numBits, writer.position());
      writer.flush();
      TEST_ASSERT_EQUALS(expectedData, data);
    }
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

private:
  static BitReader createGeneratorReader(std::span<const std::byte> data) {
    return BitReader{[data](std::byte &out) mutable {
      if (data.empty()) {
        return false;
      }
      out = data.front();
      data = data.subspan<1>();
      return true;
    }};
  }

  static BitWriter createConsumerWriter(std::span<std::byte> data) {
    return BitWriter{[data{std::span{data}}](std::byte in) mutable {
      if (data.empty()) {
        return false;
      }
      data.front() = in;
      data = data.subspan<1>();
      return true;
    }};
  }
};

class TestIO : public TestBase {
public:
  TestIO() : TestBase("IO") {
    TEST_ADD(TestIO::testEmptyReader);
    TEST_ADD(TestIO::testEmptyWriter);
    TEST_ADD(TestIO::testReaderEos);
    TEST_ADD(TestIO::testWriterEos);
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

  void testEmptyReader() {
    BitReader reader{};

    TEST_ASSERT_FALSE(reader.hasMoreBytes());
    TEST_ASSERT_EQUALS(0_bits, reader.position());
    TEST_THROWS(reader.skipToAligment(8_bits), std::runtime_error);
    TEST_THROWS(reader.assertAlignment(1_bytes), std::runtime_error);
    TEST_THROWS(reader.read(), std::runtime_error);
    TEST_THROWS(reader.peek(1_bytes), std::runtime_error);
    TEST_THROWS(reader.read(1_bytes), std::runtime_error);
    TEST_THROWS(reader.readBytes(1_bytes), std::runtime_error);
    TEST_THROWS(reader.readByte(), std::runtime_error);
    std::vector<std::byte> data(45);
    TEST_THROWS(reader.readBytesInto(data), std::runtime_error);
    TEST_THROWS(reader.readExpGolomb(), std::runtime_error);
    TEST_THROWS(reader.readSignedExpGolomb(), std::runtime_error);
    TEST_THROWS(reader.readUtf8CodePoint(), std::runtime_error);
    TEST_THROWS(reader.readUtf16CodePoint(), std::runtime_error);
    TEST_THROWS(reader.readFibonacci(), std::runtime_error);
    TEST_THROWS(reader.readSignedFibonacci(), std::runtime_error);
    TEST_THROWS(reader.skip(1_bytes), std::runtime_error);
    TEST_ASSERT_FALSE(reader.hasMoreBytes());
  }

  void testEmptyWriter() {
    BitWriter writer{};

    TEST_ASSERT_EQUALS(0_bits, writer.position());
    TEST_THROWS(writer.fillToAligment(8_bits, false), std::runtime_error);
    TEST_THROWS(writer.assertAlignment(1_bytes), std::runtime_error);
    TEST_THROWS(writer.write(true), std::runtime_error);
    TEST_THROWS(writer.write(42, 17_bits), std::runtime_error);
    TEST_THROWS(writer.writeBytes(42, 3_bytes), std::runtime_error);
    TEST_THROWS(writer.writeByte(std::byte{17}), std::runtime_error);
    std::vector<std::byte> data(45);
    TEST_THROWS(writer.writeBytes(data), std::runtime_error);
    TEST_THROWS(writer.writeExpGolomb(42), std::runtime_error);
    TEST_THROWS(writer.writeSignedExpGolomb(-420), std::runtime_error);
    TEST_THROWS(writer.writeUtf8CodePoint(U'⠁'), std::runtime_error);
    TEST_THROWS(writer.writeUtf16CodePoint(U'⠁'), std::runtime_error);
    TEST_THROWS(writer.writeFibonacci(42), std::runtime_error);
    TEST_THROWS(writer.writeSignedFibonacci(-14), std::runtime_error);
    TEST_THROWS(writer.flush(), std::runtime_error);
  }

  void testReaderEos() {
    BitReader reader1{toBytes({})};
    BitReader reader2{[](std::byte &) { return false; }};

    for (auto *reader : {&reader1, &reader2}) {
      TEST_ASSERT_EQUALS(0_bits, reader->position());
      TEST_ASSERT_FALSE(reader->hasMoreBytes());
      TEST_ASSERT_EQUALS(0_bits, reader->skipToAligment(1_bytes));
      TEST_THROWS_NOTHING(reader->assertAlignment(1_bytes));
      TEST_THROWS(reader->read(), EndOfStreamError);
      TEST_THROWS(reader->peek(7_bits), EndOfStreamError);
      TEST_THROWS(reader->read(3_bits), EndOfStreamError);
      TEST_THROWS(reader->readBytes(2_bytes), EndOfStreamError);
      TEST_THROWS(reader->readByte(), EndOfStreamError);
      std::vector<std::byte> data(42);
      TEST_THROWS(reader->readBytesInto(data), EndOfStreamError);
      TEST_THROWS(reader->readExpGolomb(), EndOfStreamError);
      TEST_THROWS(reader->readSignedExpGolomb(), EndOfStreamError);
      TEST_THROWS(reader->readUtf8CodePoint(), EndOfStreamError);
      TEST_THROWS(reader->readUtf16CodePoint(), EndOfStreamError);
      TEST_THROWS(reader->readFibonacci(), EndOfStreamError);
      TEST_THROWS(reader->readSignedFibonacci(), EndOfStreamError);
      TEST_THROWS(reader->skip(1_bytes), EndOfStreamError);
      TEST_ASSERT_FALSE(reader->hasMoreBytes());
    }
  }

  void testWriterEos() {
    std::vector<std::byte> buffer{};
    BitWriter writer1{buffer};
    BitWriter writer2{[](std::byte) { return false; }};

    for (auto *writer : {&writer1, &writer2}) {
      TEST_ASSERT_EQUALS(0_bits, writer->position());
      TEST_ASSERT_EQUALS(0_bits, writer->fillToAligment(8_bits, false));
      TEST_THROWS_NOTHING(writer->assertAlignment(1_bytes));
      // is cached, not yet written back to the sink
      TEST_THROWS_NOTHING(writer->write(true));
      TEST_THROWS(writer->flush(), EndOfStreamError);
      TEST_THROWS(writer->write(42, 16_bits), EndOfStreamError);
      TEST_THROWS(writer->writeBytes(42, 3_bytes), EndOfStreamError);
      TEST_THROWS(writer->writeByte(std::byte{17}), EndOfStreamError);
      std::vector<std::byte> data(45);
      TEST_THROWS(writer->writeBytes(data), EndOfStreamError);
      TEST_THROWS(writer->writeExpGolomb(42), EndOfStreamError);
      TEST_THROWS(writer->writeSignedExpGolomb(-420), EndOfStreamError);
      TEST_THROWS(writer->flush(), EndOfStreamError);
      TEST_THROWS(writer->writeUtf8CodePoint(U'⠁'), EndOfStreamError);
      TEST_THROWS(writer->writeUtf16CodePoint(U'⠁'), EndOfStreamError);
      TEST_THROWS(writer->writeFibonacci(42), EndOfStreamError);
      TEST_THROWS(writer->writeSignedFibonacci(-14), EndOfStreamError);
      TEST_THROWS(writer->flush(), EndOfStreamError);
    }
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
    const std::array<EnumType, 5> DATA_BUFFER{EnumType::BAR, EnumType::FOO, EnumType::BAZ, EnumType::FOO,
                                              EnumType::BAR};
    const std::span DATA{DATA_BUFFER};

    checkRead(EnumType::BAR, std::as_bytes(DATA.subspan(0, 1)));
    checkRead(DATA_BUFFER, std::as_bytes(DATA));
    static_assert(!Readable<std::optional<EnumType>>);
    static_assert(!Readable<std::unique_ptr<EnumType>>);
    static_assert(!Readable<std::vector<EnumType>>);

    checkWrite(EnumType::BAR, std::as_bytes(DATA.subspan(0, 1)));
    checkWrite(DATA_BUFFER, std::as_bytes(DATA));
    checkWrite(std::optional<EnumType>{}, {}, 0_bytes);
    checkWrite(std::optional<EnumType>{EnumType::BAR}, std::as_bytes(DATA.subspan(0, 1)), 1_bytes);
    checkWrite(std::unique_ptr<EnumType>{}, {}, 0_bytes);
    checkWrite(std::make_unique<EnumType>(EnumType::BAR), std::as_bytes(DATA.subspan(0, 1)), 1_bytes);
    checkWrite(std::vector<EnumType>{}, {}, 0_bytes);
    checkWrite(std::vector<EnumType>{DATA.begin(), DATA.end()}, std::as_bytes(DATA), 5_bytes);

    checkSkip<EnumType>(std::as_bytes(DATA));
    checkSkip<std::array<EnumType, 5>>(std::as_bytes(DATA));
    static_assert(!Skipable<std::optional<EnumType>>);
    static_assert(!Skipable<std::unique_ptr<EnumType>>);
    static_assert(!Skipable<std::vector<EnumType>>);

    checkCopy<EnumType>(std::as_bytes(DATA.subspan(0, 1)));
    checkCopy<std::array<EnumType, 5>>(std::as_bytes(DATA));
    static_assert(!Copyable<std::optional<EnumType>>);
    static_assert(!Copyable<std::unique_ptr<EnumType>>);
    static_assert(!Copyable<std::vector<EnumType>>);

    checkPrint(EnumType::BAZ, "32");
    checkPrint(DATA_BUFFER, "5 [17, 1, 32, 1, 17, ]");
    checkPrint(std::optional<EnumType>{}, "(none)");
    checkPrint(std::optional<EnumType>{EnumType::BAZ}, "32");
    checkPrint(std::unique_ptr<EnumType>{}, "(none)");
    checkPrint(std::make_unique<EnumType>(EnumType::BAR), "17");
    checkPrint(std::vector<EnumType>{}, "0 []");
    checkPrint(std::vector<EnumType>{DATA.begin(), DATA.end()}, "5 [17, 1, 32, 1, 17, ]");

    checkSize(EnumType::BAZ, 1_bytes, true);
    checkSize(DATA_BUFFER, 5_bytes, true);
    checkSize(std::optional<EnumType>{}, 0_bytes, false);
    checkSize(std::optional<EnumType>{EnumType::FOO}, 1_bytes, false);
    checkSize(std::unique_ptr<EnumType>{}, 0_bytes, false);
    checkSize(std::make_unique<EnumType>(EnumType::BAR), 1_bytes, false);
    checkSize(std::vector<EnumType>{}, 0_bytes, false);
    checkSize(std::vector<EnumType>{DATA.begin(), DATA.end()}, 5_bytes, false);
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
    using TrivialTuple = std::tuple<uint32_t, EnumType>;
    using PodTuple = std::tuple<POD, uint32_t, EnumType>;
    using MemberTuple = std::tuple<Member, uint32_t, EnumType>;

    const auto TRIVIAL_DATA = toBytes({0x00, 0x01, 0xB9, 0xD9, 0x20});
    const auto POD_DATA = toBytes({0xFF, 0xFF, 0xFF, 0xF3, 0x00, 0x12, 0xD6, 0x87, 0x34, 0x00, 0x01, 0xB5, 0xF1, 0x11});
    const auto MEMBER_DATA = toBytes({0x11, 0x01, 0x90, 0xFD, 0x00, 0x0F, 0x12, 0x06, 0x01});

    const TrivialTuple TRIVIAL{113113, EnumType::BAZ};
    const PodTuple PODV{{-13, 1234567, '4'}, 112113, EnumType::BAR};
    const MemberTuple MEMBER{{17, 400, -3}, 987654, EnumType::FOO};

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
    using TrivialVariant = std::variant<uint32_t, EnumType>;
    using TrivialLargeVariant = std::variant<uint32_t, LargeEnum>;
    using PodVariant = std::variant<POD, uint32_t, EnumType>;
    using MemberVariant = std::variant<Member, uint32_t, EnumType>;

    const std::vector<std::byte> TRIVIAL_DATA{std::byte{17}};
    const auto LARGE_DATA = toBytes({0x00, 0x01, 0xC9, 0x7E});
    const auto POD_DATA = toBytes({0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x0E, 0x37});
    const auto MEMBER_DATA = toBytes({0x11, 0x00, 0x16, 0x2D});

    const TrivialVariant TRIVIAL{EnumType::BAR};
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
    TEST_ADD(TestTypes::testSignedBytes);
    TEST_ADD(TestTypes::testFixedBits);
    TEST_ADD(TestTypes::testFixedBytes);
    TEST_ADD(TestTypes::testFixedByte);
    TEST_ADD(TestTypes::testChar);
    TEST_ADD(TestTypes::testExpGolombBits);
    TEST_ADD(TestTypes::testSignedExpGolombBits);
    TEST_ADD(TestTypes::testUtf8CodePoint);
    TEST_ADD(TestTypes::testUtf16CodePoint);
    TEST_ADD(TestTypes::testUtf32CodePoint);
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
    checkValueBitfield<Bits<8, EnumType>>(EnumType::FOO, toBytes({0x01}), "0x01", 8_bits, true);
  }

  void testBytes() {
    checkValueBitfield<Bytes<3>, uint32_t>(123456U, toBytes({0x01, 0xE2, 0x40}), "0x01e240", 3_bytes, true);
    checkValueBitfield<Bytes<7>, uint64_t>(0x91A2B3C4855E6FULL, toBytes({0x91, 0xA2, 0xB3, 0xC4, 0x85, 0x5E, 0x6F}),
                                           "0x91a2b3c4855e6f", 7_bytes, true);
  }

  void testByte() {
    checkValueBitfield<Byte<>>(uint8_t{17}, toBytes({0x11}), "0x11", 1_bytes, true);
    checkValueBitfield<Byte<EnumType>>(EnumType::BAR, toBytes({0x11}), "0x11", 1_bytes, true);
  }

  void testSignedBytes() {
    checkValueBitfield<SignedBytes<int8_t>>(int8_t{-1}, toBytes({0xFF}), "0xff", 1_bytes, true);
    checkValueBitfield<SignedBytes<int16_t>>(int16_t{2}, toBytes({0x00, 0x02}), "0x0002", 2_bytes, true);
    checkValueBitfield<SignedBytes<int32_t>>(-123456, toBytes({0xFF, 0xFE, 0x1D, 0xC0}), "0xfffe1dc0", 4_bytes, true);
    checkValueBitfield<SignedBytes<int64_t>>(-0x91A2B3C4855E6FL,
                                             toBytes({0xFF, 0x6E, 0x5D, 0x4C, 0x3B, 0x7A, 0xA1, 0x91}),
                                             "0xff6e5d4c3b7aa191", 8_bytes, true);
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
    checkBitfield(FixedByte<EnumType::BAR>{}, toBytes({0x11}), "0x11", 1_bytes, true);
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

  void testUtf8CodePoint() {
    checkValueBitfield<Utf8CodePoint>(U'\0', toBytes({0x00}), "\\0", 1_bytes, false);
    checkValueBitfield<Utf8CodePoint>(U'A', toBytes({0x41}), "A", 1_bytes, false);
    checkValueBitfield<Utf8CodePoint>(U'£', toBytes({0xC2, 0xA3}), "£", 2_bytes, false);
    checkValueBitfield<Utf8CodePoint>(U'÷', toBytes({0xC3, 0xB7}), "÷", 2_bytes, false);
    checkValueBitfield<Utf8CodePoint>(U'ᴀ', toBytes({0xE1, 0xB4, 0x80}), "ᴀ", 3_bytes, false); // small capital A
    checkValueBitfield<Utf8CodePoint>(U'₤', toBytes({0xE2, 0x82, 0xA4}), "₤", 3_bytes, false); // Lira
    checkValueBitfield<Utf8CodePoint>(U'𐞃', toBytes({0xF0, 0x90, 0x9E, 0x83}), "𐞃", 4_bytes, false);
    checkValueBitfield<Utf8CodePoint>(U'🚆', toBytes({0xF0, 0x9F, 0x9A, 0x86}), "🚆", 4_bytes, false); // train
    checkValueBitfield<Utf8CodePoint>(U'🤍', toBytes({0xF0, 0x9F, 0xA4, 0x8D}), "🤍", 4_bytes, false); // white heart
  }

  void testUtf16CodePoint() {
    checkValueBitfield<Utf16CodePoint>(U'\0', toBytes({0x00, 0x00}), "\\0", 2_bytes, false);
    checkValueBitfield<Utf16CodePoint>(U'A', toBytes({0x00, 0x41}), "A", 2_bytes, false);
    checkValueBitfield<Utf16CodePoint>(U'£', toBytes({0x00, 0xA3}), "£", 2_bytes, false);
    checkValueBitfield<Utf16CodePoint>(U'÷', toBytes({0x00, 0xF7}), "÷", 2_bytes, false);
    checkValueBitfield<Utf16CodePoint>(U'ᴀ', toBytes({0x1D, 0x00}), "ᴀ", 2_bytes, false); // small capital A
    checkValueBitfield<Utf16CodePoint>(U'₤', toBytes({0x20, 0xA4}), "₤", 2_bytes, false); // Lira
    checkValueBitfield<Utf16CodePoint>(U'𐞃', toBytes({0xD8, 0x01, 0xDF, 0x83}), "𐞃", 4_bytes, false);
    checkValueBitfield<Utf16CodePoint>(U'🚆', toBytes({0xD8, 0x3D, 0xDE, 0x86}), "🚆", 4_bytes, false); // train
    checkValueBitfield<Utf16CodePoint>(U'🤍', toBytes({0xD8, 0x3E, 0xDD, 0x0D}), "🤍", 4_bytes, false); // white heart
  }

  void testUtf32CodePoint() {
    checkValueBitfield<Utf32CodePoint>(U'\0', toBytes({0x00, 0x00, 0x00, 0x00}), "\\0", 4_bytes, true);
    checkValueBitfield<Utf32CodePoint>(U'A', toBytes({0x00, 0x00, 0x00, 0x41}), "A", 4_bytes, true);
    checkValueBitfield<Utf32CodePoint>(U'£', toBytes({0x00, 0x00, 0x00, 0xA3}), "£", 4_bytes, true);
    checkValueBitfield<Utf32CodePoint>(U'÷', toBytes({0x00, 0x00, 0x00, 0xF7}), "÷", 4_bytes, true);
    checkValueBitfield<Utf32CodePoint>(U'ᴀ', toBytes({0x00, 0x00, 0x1D, 0x00}), "ᴀ", 4_bytes, true); // small capital A
    checkValueBitfield<Utf32CodePoint>(U'₤', toBytes({0x00, 0x00, 0x20, 0xA4}), "₤", 4_bytes, true); // Lira
    checkValueBitfield<Utf32CodePoint>(U'𐞃', toBytes({0x00, 0x01, 0x07, 0x83}), "𐞃", 4_bytes, true);
    checkValueBitfield<Utf32CodePoint>(U'🚆', toBytes({0x00, 0x01, 0xF6, 0x86}), "🚆", 4_bytes, true); // train
    checkValueBitfield<Utf32CodePoint>(U'🤍', toBytes({0x00, 0x01, 0xF9, 0x0D}), "🤍", 4_bytes, true); // white heart
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

void registerTypeTests() {
  Test::registerSuite(Test::newInstance<TestIO>, "io");
  Test::registerSuite(Test::newInstance<TestTypes>, "types");
}
