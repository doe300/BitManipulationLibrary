#include "ebml.hpp"
#include "ebml_common.hpp"
#include "errors.hpp"

#include "cpptest-main.h"

#include <deque>

using namespace bml;
using namespace bml::ebml;

struct TestDynamicElement final : MasterElement {
  static constexpr ElementId ID{0xF0};

  constexpr auto operator<=>(const TestDynamicElement &) const noexcept = default;

  void read(bml::BitReader &reader, const ReadOptions &options = {}) {
    bml::ebml::detail::readMasterElement(
        reader, ID, *this, options,
        {bml::ebml::detail::wrapMemberReader(crc32), bml::ebml::detail::wrapMemberReader(voidElements)}, {ID});
  }

  ChunkedReader readChunked(bml::BitReader &reader, const ReadOptions &options = {}) {
    return bml::ebml::detail::readMasterElementChunked(
        reader, ID, *this, options,
        {bml::ebml::detail::wrapMemberReader(crc32), bml::ebml::detail::wrapMemberReader(voidElements)}, {ID});
  }

  void write(bml::BitWriter &writer) const {
    bml::ebml::detail::writeMasterElement(writer, ID, crc32);
    bml::ebml::detail::writeMasterElement(writer, ID, voidElements);
  }

  BML_DEFINE_PRINT(TestDynamicElement, crc32, voidElements)

  static void skip(BitReader &reader) { bml::ebml::detail::skipElement(reader, {ID}); }
  static void copy(BitReader &reader, BitWriter &writer) { bml::ebml::detail::copyElement(reader, writer, {ID}); }
};

enum class TestEnum : uint8_t {
  FOO = 1,
  BAR = 2,
  BAZ = 177,
};

class TestBaseElements : public TestElementsBase {
public:
  TestBaseElements() : TestElementsBase("BaseElements") {
    TEST_ADD(TestBaseElements::testVariableSizeInteger);
    TEST_ADD(TestBaseElements::testBoolElement);
    TEST_ADD(TestBaseElements::testSignedIntElement);
    TEST_ADD(TestBaseElements::testUnsignedIntElement);
    TEST_ADD(TestBaseElements::testEnumElement);
    TEST_ADD(TestBaseElements::testFloatElement);
    TEST_ADD(TestBaseElements::testDateElement);
    TEST_ADD(TestBaseElements::testAsciiStringElement);
    TEST_ADD(TestBaseElements::testUtf8StringElement);
    TEST_ADD(TestBaseElements::testBinaryElement);
    TEST_ADD(TestBaseElements::testIDMismatch);
    TEST_ADD(TestBaseElements::testCRC32Mismatch);
    TEST_ADD(TestBaseElements::testUnknownElement);
    TEST_ADD(TestBaseElements::testUnknownDataSize);
    TEST_ADD(TestBaseElements::testChunkedReadEmpty);
    TEST_ADD(TestBaseElements::testChunkedReadFillJustEnough);
    TEST_ADD(TestBaseElements::testChunkedReadUnknownDataSize);
    TEST_ADD(TestBaseElements::testChunkedReadPrematureEos);
    TEST_ADD(TestBaseElements::testChunkedReadPartial);
    TEST_ADD(TestBaseElements::testChunkedReadUnsymmetricUsage);
    TEST_ADD(TestBaseElements::testConcurrentChunkedReaders);
  }

  void testVariableSizeInteger() {
    VariableSizeInteger elem{};
    static_assert(VariableSizeInteger::maxNumBits() == 8_bytes);

    testValueBitField(elem, 0U, {0x80}, "0", 1_bytes);
    elem.set(112);
    testValueBitField(elem, elem.get(), {0xF0}, "112", 1_bytes);

    elem.set(34545234);
    testValueBitField(elem, elem.get(), {0x12, 0x0F, 0x1E, 0x52}, "34545234", 4_bytes);
  }

  void testBoolElement() {
    BoolElement<0xAB_id> elem{};
    static_assert(BoolElement<0xAB_id>::maxNumBits() == 3_bytes);

    testValueElementDefaultValue(elem, {0xAB, 0x80}, "0", 2_bytes);
    elem.set(true);
    testValueElement(elem, elem.get(), {0xAB, 0x81, 0x01}, "1", 3_bytes);
  }

  void testSignedIntElement() {
    SignedIntElement<0x45A3_id> elem{};
    static_assert(SignedIntElement<0x45A3_id>::maxNumBits() == 11_bytes);

    testValueElementDefaultValue(elem, {0x45, 0xA3, 0x80}, "0", 3_bytes);
    elem.set(17);
    testValueElement(elem, elem.get(), {0x45, 0xA3, 0x81, 0x11}, "17", 4_bytes);
    elem.set(-17);
    testValueElement(elem, elem.get(), {0x45, 0xA3, 0x81, 0xEF}, "-17", 4_bytes);
    elem.set(300);
    testValueElement(elem, elem.get(), {0x45, 0xA3, 0x82, 0x01, 0x2C}, "300", 5_bytes);
    elem.set(-300);
    testValueElement(elem, elem.get(), {0x45, 0xA3, 0x82, 0xFE, 0xD4}, "-300", 5_bytes);
    elem.set(0x77777777);
    testValueElement(elem, elem.get(), {0x45, 0xA3, 0x84, 0x77, 0x77, 0x77, 0x77}, "2004318071", 7_bytes);
    elem.set(-0x77777777);
    testValueElement(elem, elem.get(), {0x45, 0xA3, 0x84, 0x88, 0x88, 0x88, 0x89}, "-2004318071", 7_bytes);
    elem.set(0x7FFFFFFFFFFFFFFFLL);
    testValueElement(elem, elem.get(), {0x45, 0xA3, 0x88, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                     "9223372036854775807", 11_bytes);
    elem.set(std::bit_cast<int64_t>(0x8000000000000000ULL));
    testValueElement(elem, elem.get(), {0x45, 0xA3, 0x88, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                     "-9223372036854775808", 11_bytes);
  }

  void testUnsignedIntElement() {
    UnsignedIntElement<0x45A3_id> elem{};
    static_assert(UnsignedIntElement<0x45A3_id>::maxNumBits() == 11_bytes);

    testValueElementDefaultValue(elem, {0x45, 0xA3, 0x80}, "0", 3_bytes);
    elem.set(17);
    testValueElement(elem, elem.get(), {0x45, 0xA3, 0x81, 0x11}, "17", 4_bytes);
    elem.set(0x77777777U);
    testValueElement(elem, elem.get(), {0x45, 0xA3, 0x84, 0x77, 0x77, 0x77, 0x77}, "2004318071", 7_bytes);
    elem.set(0xFFFFFFFFFFFFFFFFULL);
    testValueElement(elem, elem.get(), {0x45, 0xA3, 0x88, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                     "18446744073709551615", 11_bytes);
  }

  void testEnumElement() {
    EnumElement<0x45A3_id, TestEnum, TestEnum::BAR> elem{};
    static_assert(EnumElement<0x45A3_id, TestEnum>::maxNumBits() == 4_bytes);

    testValueElementDefaultValue(elem, {0x45, 0xA3, 0x80}, "2", 3_bytes, TestEnum::BAR);
    elem.set(TestEnum::BAZ);
    testValueElement(elem, elem.get(), {0x45, 0xA3, 0x81, 0xB1}, "177", 4_bytes);
    elem.set(TestEnum::FOO);
    testValueElement(elem, elem.get(), {0x45, 0xA3, 0x81, 0x01}, "1", 4_bytes);
  }

  void testFloatElement() {
    FloatElement<0x7373_id, double> doubleElem{};
    static_assert(FloatElement<0x7373_id, double>::maxNumBits() == 11_bytes);

    testValueElementDefaultValue(doubleElem, {0x73, 0x73, 0x80}, "0", 3_bytes);

    doubleElem.set(-17.4);
    testValueElement(doubleElem, doubleElem.get(), {0x73, 0x73, 0x88, 0xC0, 0x31, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66},
                     "-17.4", 11_bytes);

    FloatElement<0x7373_id, float> floatElem{};
    static_assert(FloatElement<0x7373_id, float>::maxNumBits() == 7_bytes);

    testValueElementDefaultValue(floatElem, {0x73, 0x73, 0x80}, "0", 3_bytes);

    floatElem.set(-17.4F);
    testValueElement(floatElem, floatElem.get(), {0x73, 0x73, 0x84, 0xC1, 0x8B, 0x33, 0x33}, "-17.4", 7_bytes);
  }

  void testDateElement() {
    DateElement<0x45A3_id> elem{};
    static_assert(DateElement<0x45A3_id>::maxNumBits() == 11_bytes);

    const auto toDate = [](intmax_t val) { return bml::ebml::detail::DATE_EPOCH + std::chrono::nanoseconds{val}; };

    testValueElementDefaultValue(elem, {0x45, 0xA3, 0x80}, "2001-01-01 00:00:00.000000000", 3_bytes,
                                 ebml::detail::DATE_EPOCH);
    elem.set(toDate(17));
    testValueElement(elem, elem.get(), {0x45, 0xA3, 0x81, 0x11}, "2001-01-01 00:00:00.000000017", 4_bytes);
    elem.set(toDate(-17));
    testValueElement(elem, elem.get(), {0x45, 0xA3, 0x81, 0xEF}, "2000-12-31 23:59:59.999999983", 4_bytes);
    elem.set(toDate(300));
    testValueElement(elem, elem.get(), {0x45, 0xA3, 0x82, 0x01, 0x2C}, "2001-01-01 00:00:00.000000300", 5_bytes);
    elem.set(toDate(-300));
    testValueElement(elem, elem.get(), {0x45, 0xA3, 0x82, 0xFE, 0xD4}, "2000-12-31 23:59:59.999999700", 5_bytes);
    elem.set(toDate(0x77777777));
    testValueElement(elem, elem.get(), {0x45, 0xA3, 0x84, 0x77, 0x77, 0x77, 0x77}, "2001-01-01 00:00:02.004318071",
                     7_bytes);
    elem.set(toDate(-0x77777777));
    testValueElement(elem, elem.get(), {0x45, 0xA3, 0x84, 0x88, 0x88, 0x88, 0x89}, "2000-12-31 23:59:57.995681929",
                     7_bytes);
  }

  void testAsciiStringElement() {
    StringElement<0x1254C367_id> elem{};
    static_assert(StringElement<0x1254C367_id>::maxNumBits() >= ByteCount{1ULL << 56UL});

    testValueElementDefaultValue(elem, {0x12, 0x54, 0xC3, 0x67, 0x80}, "", 5_bytes);

    elem.set("FOO BAR");
    testValueElement(elem, elem.get(), {0x12, 0x54, 0xC3, 0x67, 0x87, 0x46, 0x4F, 0x4F, 0x20, 0x42, 0x41, 0x52},
                     "FOO BAR", 12_bytes);

    const char text[] =
        "This text has exactly 126 characters to test the Variable-size Integer handling regarding 7 valid bits "
        "in an 8 bit byte.......";
    static_assert(std::extent_v<decltype(text)> == 127 /* with NUL-byte */);
    elem.set(text);
    testValueElement(elem, elem.get(),
                     {0x12, 0x54, 0xC3, 0x67, 0xFE, 0x54, 0x68, 0x69, 0x73, 0x20, 0x74, 0x65, 0x78, 0x74, 0x20,
                      0x68, 0x61, 0x73, 0x20, 0x65, 0x78, 0x61, 0x63, 0x74, 0x6c, 0x79, 0x20, 0x31, 0x32, 0x36,
                      0x20, 0x63, 0x68, 0x61, 0x72, 0x61, 0x63, 0x74, 0x65, 0x72, 0x73, 0x20, 0x74, 0x6f, 0x20,
                      0x74, 0x65, 0x73, 0x74, 0x20, 0x74, 0x68, 0x65, 0x20, 0x56, 0x61, 0x72, 0x69, 0x61, 0x62,
                      0x6c, 0x65, 0x2d, 0x73, 0x69, 0x7a, 0x65, 0x20, 0x49, 0x6e, 0x74, 0x65, 0x67, 0x65, 0x72,
                      0x20, 0x68, 0x61, 0x6e, 0x64, 0x6c, 0x69, 0x6e, 0x67, 0x20, 0x72, 0x65, 0x67, 0x61, 0x72,
                      0x64, 0x69, 0x6e, 0x67, 0x20, 0x37, 0x20, 0x76, 0x61, 0x6c, 0x69, 0x64, 0x20, 0x62, 0x69,
                      0x74, 0x73, 0x20, 0x69, 0x6e, 0x20, 0x61, 0x6e, 0x20, 0x38, 0x20, 0x62, 0x69, 0x74, 0x20,
                      0x62, 0x79, 0x74, 0x65, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e},
                     text, 4_bytes + 1_bytes + 126_bytes);

    std::string modifiedText = text;
    modifiedText[24] = '7';
    modifiedText.push_back('!');
    TEST_ASSERT_EQUALS(127U, modifiedText.size());
    elem.set(modifiedText);
    testValueElement(elem, elem.get(),
                     {0x12, 0x54, 0xC3, 0x67, 0x40, 0x7F, 0x54, 0x68, 0x69, 0x73, 0x20, 0x74, 0x65, 0x78, 0x74,
                      0x20, 0x68, 0x61, 0x73, 0x20, 0x65, 0x78, 0x61, 0x63, 0x74, 0x6c, 0x79, 0x20, 0x31, 0x32,
                      0x37, 0x20, 0x63, 0x68, 0x61, 0x72, 0x61, 0x63, 0x74, 0x65, 0x72, 0x73, 0x20, 0x74, 0x6f,
                      0x20, 0x74, 0x65, 0x73, 0x74, 0x20, 0x74, 0x68, 0x65, 0x20, 0x56, 0x61, 0x72, 0x69, 0x61,
                      0x62, 0x6c, 0x65, 0x2d, 0x73, 0x69, 0x7a, 0x65, 0x20, 0x49, 0x6e, 0x74, 0x65, 0x67, 0x65,
                      0x72, 0x20, 0x68, 0x61, 0x6e, 0x64, 0x6c, 0x69, 0x6e, 0x67, 0x20, 0x72, 0x65, 0x67, 0x61,
                      0x72, 0x64, 0x69, 0x6e, 0x67, 0x20, 0x37, 0x20, 0x76, 0x61, 0x6c, 0x69, 0x64, 0x20, 0x62,
                      0x69, 0x74, 0x73, 0x20, 0x69, 0x6e, 0x20, 0x61, 0x6e, 0x20, 0x38, 0x20, 0x62, 0x69, 0x74,
                      0x20, 0x62, 0x79, 0x74, 0x65, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x21},
                     modifiedText, 4_bytes + 2_bytes + 127_bytes);

    const char longText[] =
        "This text has exactly 255 characters to test the Variable-size Integer handling regarding 7 valid bits "
        "in an 8 bit byte. To achieve this, we add some more dummy data and stuff and text and more words as "
        "fillers and padding and so on until we reach the 255";
    static_assert(std::extent_v<decltype(longText)> == 256 /* with NUL-byte */);
    elem.set(longText);
    testValueElement(
        elem, elem.get(),
        {0x12, 0x54, 0xC3, 0x67, 0x40, 0xFF, 0x54, 0x68, 0x69, 0x73, 0x20, 0x74, 0x65, 0x78, 0x74, 0x20, 0x68, 0x61,
         0x73, 0x20, 0x65, 0x78, 0x61, 0x63, 0x74, 0x6c, 0x79, 0x20, 0x32, 0x35, 0x35, 0x20, 0x63, 0x68, 0x61, 0x72,
         0x61, 0x63, 0x74, 0x65, 0x72, 0x73, 0x20, 0x74, 0x6f, 0x20, 0x74, 0x65, 0x73, 0x74, 0x20, 0x74, 0x68, 0x65,
         0x20, 0x56, 0x61, 0x72, 0x69, 0x61, 0x62, 0x6c, 0x65, 0x2d, 0x73, 0x69, 0x7a, 0x65, 0x20, 0x49, 0x6e, 0x74,
         0x65, 0x67, 0x65, 0x72, 0x20, 0x68, 0x61, 0x6e, 0x64, 0x6c, 0x69, 0x6e, 0x67, 0x20, 0x72, 0x65, 0x67, 0x61,
         0x72, 0x64, 0x69, 0x6e, 0x67, 0x20, 0x37, 0x20, 0x76, 0x61, 0x6c, 0x69, 0x64, 0x20, 0x62, 0x69, 0x74, 0x73,
         0x20, 0x69, 0x6e, 0x20, 0x61, 0x6e, 0x20, 0x38, 0x20, 0x62, 0x69, 0x74, 0x20, 0x62, 0x79, 0x74, 0x65, 0x2e,
         0x20, 0x54, 0x6f, 0x20, 0x61, 0x63, 0x68, 0x69, 0x65, 0x76, 0x65, 0x20, 0x74, 0x68, 0x69, 0x73, 0x2c, 0x20,
         0x77, 0x65, 0x20, 0x61, 0x64, 0x64, 0x20, 0x73, 0x6f, 0x6d, 0x65, 0x20, 0x6d, 0x6f, 0x72, 0x65, 0x20, 0x64,
         0x75, 0x6d, 0x6d, 0x79, 0x20, 0x64, 0x61, 0x74, 0x61, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x73, 0x74, 0x75, 0x66,
         0x66, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x74, 0x65, 0x78, 0x74, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x6d, 0x6f, 0x72,
         0x65, 0x20, 0x77, 0x6f, 0x72, 0x64, 0x73, 0x20, 0x61, 0x73, 0x20, 0x66, 0x69, 0x6c, 0x6c, 0x65, 0x72, 0x73,
         0x20, 0x61, 0x6e, 0x64, 0x20, 0x70, 0x61, 0x64, 0x64, 0x69, 0x6e, 0x67, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x73,
         0x6f, 0x20, 0x6f, 0x6e, 0x20, 0x75, 0x6e, 0x74, 0x69, 0x6c, 0x20, 0x77, 0x65, 0x20, 0x72, 0x65, 0x61, 0x63,
         0x68, 0x20, 0x74, 0x68, 0x65, 0x20, 0x32, 0x35, 0x35},
        longText, 4_bytes + 2_bytes + 255_bytes);
  }

  void testUtf8StringElement() {
    Utf8StringElement<0x3EB923_id> elem{};
    static_assert(Utf8StringElement<0x3EB923_id>::maxNumBits() >= ByteCount{1ULL << 56UL});

    testValueElementDefaultValue(elem, {0x3E, 0xB9, 0x23, 0x80}, "''", 4_bytes);

    elem.set(u8"⠁⠂⠃⠄⠅⠆⠇⠈ 🂠 🂡🂢🂣🂤🂥🂦🂧🂨🂩🂪🂫🂬🂭🂮 🂱🂲🂳🂴🂵🂶🂷🂸🂹🂺🂻🂼🂽🂾 🃁🃂🃃🃄🃅🃆🃇🃈🃉🃊🃋🃌🃍🃎 🃑🃒🃓🃔🃕🃖🃗🃘🃙🃚🃛🃜🃝🃞");
    testValueElement(elem, elem.get(),
                     {0x3E, 0xB9, 0x23, 0x41, 0x01,
                      // Braille
                      0xe2, 0xa0, 0x81, 0xe2, 0xa0, 0x82, 0xe2, 0xa0, 0x83, 0xe2, 0xa0, 0x84, 0xe2, 0xa0, 0x85, 0xe2,
                      0xa0, 0x86, 0xe2, 0xa0, 0x87, 0xe2, 0xa0, 0x88, 0x20,
                      // Playing cards
                      0xf0, 0x9f, 0x82, 0xa0, 0x20,
                      // Spades
                      0xf0, 0x9f, 0x82, 0xa1, 0xf0, 0x9f, 0x82, 0xa2, 0xf0, 0x9f, 0x82, 0xa3, 0xf0, 0x9f, 0x82, 0xa4,
                      0xf0, 0x9f, 0x82, 0xa5, 0xf0, 0x9f, 0x82, 0xa6, 0xf0, 0x9f, 0x82, 0xa7, 0xf0, 0x9f, 0x82, 0xa8,
                      0xf0, 0x9f, 0x82, 0xa9, 0xf0, 0x9f, 0x82, 0xaa, 0xf0, 0x9f, 0x82, 0xab, 0xf0, 0x9f, 0x82, 0xac,
                      0xf0, 0x9f, 0x82, 0xad, 0xf0, 0x9f, 0x82, 0xae, 0x20,
                      // Hearts
                      0xf0, 0x9f, 0x82, 0xb1, 0xf0, 0x9f, 0x82, 0xb2, 0xf0, 0x9f, 0x82, 0xb3, 0xf0, 0x9f, 0x82, 0xb4,
                      0xf0, 0x9f, 0x82, 0xb5, 0xf0, 0x9f, 0x82, 0xb6, 0xf0, 0x9f, 0x82, 0xb7, 0xf0, 0x9f, 0x82, 0xb8,
                      0xf0, 0x9f, 0x82, 0xb9, 0xf0, 0x9f, 0x82, 0xba, 0xf0, 0x9f, 0x82, 0xbb, 0xf0, 0x9f, 0x82, 0xbc,
                      0xf0, 0x9f, 0x82, 0xbd, 0xf0, 0x9f, 0x82, 0xbe, 0x20,
                      // Diamonds
                      0xf0, 0x9f, 0x83, 0x81, 0xf0, 0x9f, 0x83, 0x82, 0xf0, 0x9f, 0x83, 0x83, 0xf0, 0x9f, 0x83, 0x84,
                      0xf0, 0x9f, 0x83, 0x85, 0xf0, 0x9f, 0x83, 0x86, 0xf0, 0x9f, 0x83, 0x87, 0xf0, 0x9f, 0x83, 0x88,
                      0xf0, 0x9f, 0x83, 0x89, 0xf0, 0x9f, 0x83, 0x8a, 0xf0, 0x9f, 0x83, 0x8b, 0xf0, 0x9f, 0x83, 0x8c,
                      0xf0, 0x9f, 0x83, 0x8d, 0xf0, 0x9f, 0x83, 0x8e, 0x20,
                      // Clubs
                      0xf0, 0x9f, 0x83, 0x91, 0xf0, 0x9f, 0x83, 0x92, 0xf0, 0x9f, 0x83, 0x93, 0xf0, 0x9f, 0x83, 0x94,
                      0xf0, 0x9f, 0x83, 0x95, 0xf0, 0x9f, 0x83, 0x96, 0xf0, 0x9f, 0x83, 0x97, 0xf0, 0x9f, 0x83, 0x98,
                      0xf0, 0x9f, 0x83, 0x99, 0xf0, 0x9f, 0x83, 0x9a, 0xf0, 0x9f, 0x83, 0x9b, 0xf0, 0x9f, 0x83, 0x9c,
                      0xf0, 0x9f, 0x83, 0x9d, 0xf0, 0x9f, 0x83, 0x9e},
                     "'⠁⠂⠃⠄⠅⠆⠇⠈ 🂠 🂡🂢🂣🂤🂥🂦🂧🂨🂩🂪🂫🂬🂭🂮 🂱🂲🂳🂴🂵🂶🂷🂸🂹🂺🂻🂼🂽🂾 🃁🃂🃃🃄🃅🃆🃇🃈🃉🃊🃋🃌🃍🃎 🃑🃒🃓🃔🃕🃖🃗🃘🃙🃚🃛🃜🃝🃞'",
                     3_bytes + 2_bytes + 257_bytes);
  }

  void testBinaryElement() {
    BinaryElement<0x7373_id> elem{};
    static_assert(BinaryElement<0x7373_id>::maxNumBits() >= bml::ebml::detail::VINTMAX);

    testValueElementDefaultValue(elem, {0x73, 0x73, 0x80}, "0 []", 3_bytes, std::vector<std::byte>{});

    elem.set({std::byte{0x01}, std::byte{0x11}, std::byte{0xFF}});
    testValueElement(elem, elem.get(), {0x73, 0x73, 0x83, 0x01, 0x11, 0xFF}, "3 [0x01, 0x11, 0xff, ]", 6_bytes);
  }

  void testIDMismatch() {
    CRC32 elem{};
    elem = 0xD1F5C58A;

    auto buffer = toBytes({0xB0, 0x84, 0xD1, 0xF5, 0xC5, 0x8A});
    bml::BitReader reader{buffer};
    TEST_THROWS(elem.read(reader), std::invalid_argument);
    TEST_ASSERT_EQUALS(0xD1F5C58AUL, elem.get());
  }

  void testCRC32Mismatch() {
    const auto DATA = toBytes({0x42, 0x81, 0x90, 0xBF, 0x84, 0xD1, 0xF5, 0xC5, 0x8A, 0x42, 0x83, 0x83, 0x46, 0x4F, 0x4F,
                               0x42, 0x84, 0x81, 0x11});

    bml::BitReader reader{DATA};
    bml::ebml::ReadOptions options{};
    options.validateCRC32 = true;

    DocTypeExtension ext{};
    TEST_THROWS(ext.read(reader, options), ChecksumMismatchError);
  }

  void testUnknownElement() {
    DocTypeExtension ext{};

    auto buffer = toBytes({0x42, 0x81, 0x8F, 0xA1 /* Block */, 0x84, 0xDE, 0xAD, 0xBE, 0xEF, 0x42, 0x83, 0x83, 0x46,
                           0x4F, 0x4F, 0x42, 0x84, 0x81, 0x11});
    bml::BitReader reader{buffer};
    ext.read(reader);
    TEST_ASSERT_EQUALS("FOO", ext.docTypeExtensionName.get());
    TEST_ASSERT_EQUALS(17U, ext.docTypeExtensionVersion.get());
  }

  void testUnknownDataSize() {
    const auto DATA = toBytes({0xF0, 0xFF, 0xBF, 0x84, 0x1B, 0xE4, 0xC4, 0x81, 0xEC, 0x83, 0x00, 0x00, 0x00, 0xEC, 0x84,
                               0x00, 0x00, 0x00, 0x00, 0xEC, 0x85, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEC, 0x88, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEC, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0xEC, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xFF, 0xEC, 0x80});

    ReadOptions options{};
    options.validateCRC32 = true;

    // Terminated by end of stream
    {
      bml::BitReader reader{std::span{DATA}.subspan(0, DATA.size() - 4U)};
      TestDynamicElement elem{};
      elem.read(reader, options);

      TEST_ASSERT(elem.crc32);
      TEST_ASSERT_EQUALS(0x81C4E41BU, elem.crc32->get());
      TEST_ASSERT_EQUALS(6U, elem.voidElements.size());
      TEST_ASSERT_EQUALS(5_bytes, elem.voidElements[2].skipBytes);
      TEST_ASSERT_FALSE(reader.hasMoreBytes());

      checkSkipElement<TestDynamicElement>(std::span{DATA}.subspan(0, DATA.size() - 4U));
      checkCopyElement<TestDynamicElement>(std::span{DATA}.subspan(0, DATA.size() - 4U));
    }

    // Terminated by other element
    {
      bml::BitReader reader{std::span{DATA}};
      TestDynamicElement elem{};
      elem.read(reader, options);

      TEST_ASSERT(elem.crc32);
      TEST_ASSERT_EQUALS(0x81C4E41BU, elem.crc32->get());
      TEST_ASSERT_EQUALS(6U, elem.voidElements.size());
      TEST_ASSERT_EQUALS(5_bytes, elem.voidElements[2].skipBytes);
      TEST_ASSERT(reader.hasMoreBytes());

      checkSkipElement<TestDynamicElement>(std::span{DATA}, 55_bytes);
    }

    // Empty due to end of stream
    {
      bml::BitReader reader{std::span{DATA}.subspan(0, 2)};
      TestDynamicElement elem{};
      elem.read(reader, options);

      TEST_ASSERT_FALSE(elem.crc32);
      TEST_ASSERT(elem.voidElements.empty());
      TEST_ASSERT_FALSE(reader.hasMoreBytes());

      checkSkipElement<TestDynamicElement>(std::span{DATA}.subspan(0, 2));
      checkCopyElement<TestDynamicElement>(std::span{DATA}.subspan(0, 2));
    }
  }

  void testChunkedReadEmpty() {
    bml::BitReader reader{std::span<const std::byte>{}};
    TestDynamicElement elem{};
    auto readMember = elem.readChunked(reader);
    TEST_ASSERT(readMember);
    TEST_THROWS(readMember(), EndOfStreamError);
    TEST_ASSERT_FALSE(readMember);
  }

  void testChunkedReadFillJustEnough() {
    // Check we can read full element when inserting just enough data to read next member Element
    std::deque<uint8_t> buffer{};

    bml::BitReader reader{[&buffer](std::byte &out) {
      if (buffer.empty()) {
        return false;
      }
      out = std::bit_cast<std::byte>(buffer.front());
      buffer.pop_front();
      return true;
    }};
    TestDynamicElement elem{};
    auto readMember = elem.readChunked(reader);
    TEST_ASSERT(readMember);

    buffer = {0xF0, 0xB5, 0xBF, 0x84, 0x1B, 0xE4, 0xC4, 0x81};
    TEST_ASSERT(readMember);
    TEST_ASSERT_EQUALS(CRC32::ID, readMember());
    TEST_ASSERT(elem.crc32);
    TEST_ASSERT_EQUALS(0x81C4E41BU, elem.crc32->get());

    buffer = {0xEC, 0x83, 0x00, 0x00, 0x00};
    TEST_ASSERT(readMember);
    TEST_ASSERT_EQUALS(Void::ID, readMember());
    TEST_ASSERT_EQUALS(3_bytes, elem.voidElements.back().skipBytes);

    buffer = {0xEC, 0x84, 0x00, 0x00, 0x00, 0x00};
    TEST_ASSERT(readMember);
    TEST_ASSERT_EQUALS(Void::ID, readMember());
    TEST_ASSERT_EQUALS(4_bytes, elem.voidElements.back().skipBytes);

    buffer = {0xEC, 0x85, 0x00, 0x00, 0x00, 0x00, 0x00};
    TEST_ASSERT(readMember);
    TEST_ASSERT_EQUALS(Void::ID, readMember());
    TEST_ASSERT_EQUALS(5_bytes, elem.voidElements.back().skipBytes);

    buffer = {0xEC, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    TEST_ASSERT(readMember);
    TEST_ASSERT_EQUALS(Void::ID, readMember());
    TEST_ASSERT_EQUALS(8_bytes, elem.voidElements.back().skipBytes);

    buffer = {0xEC, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    TEST_ASSERT(readMember);
    TEST_ASSERT_EQUALS(Void::ID, readMember());
    TEST_ASSERT_EQUALS(7_bytes, elem.voidElements.back().skipBytes);
    TEST_ASSERT(readMember);

    buffer = {0xEC, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    TEST_ASSERT_EQUALS(Void::ID, readMember());
    TEST_ASSERT_EQUALS(8_bytes, elem.voidElements.back().skipBytes);

    buffer = {0xF0, 0xFF}; // "next element"
    TEST_ASSERT(readMember);
    TEST_ASSERT_EQUALS(ElementId{}, readMember());
    TEST_ASSERT_FALSE(readMember);
  }

  void testChunkedReadUnknownDataSize() {
    const auto DATA = toBytes({0xF0, 0xFF, 0xBF, 0x84, 0x1B, 0xE4, 0xC4, 0x81, 0xEC, 0x83, 0x00, 0x00, 0x00, 0xEC,
                               0x84, 0x00, 0x00, 0x00, 0x00, 0xEC, 0x85, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEC, 0x88,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEC, 0x87, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0xEC, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});

    ReadOptions options{};
    options.validateCRC32 = true;

    // Terminated by end of stream
    bml::BitReader reader{std::span{DATA}};
    TestDynamicElement elem{};
    auto readMember = elem.readChunked(reader, options);

    TEST_ASSERT(readMember);
    TEST_ASSERT_EQUALS(CRC32::ID, readMember());
    TEST_ASSERT(elem.crc32);
    TEST_ASSERT_EQUALS(0x81C4E41BU, elem.crc32->get());

    TEST_ASSERT(readMember);
    TEST_ASSERT_EQUALS(Void::ID, readMember());
    TEST_ASSERT_EQUALS(3_bytes, elem.voidElements.back().skipBytes);
    TEST_ASSERT(readMember);
    TEST_ASSERT_EQUALS(Void::ID, readMember());
    TEST_ASSERT_EQUALS(4_bytes, elem.voidElements.back().skipBytes);
    TEST_ASSERT(readMember);
    TEST_ASSERT_EQUALS(Void::ID, readMember());
    TEST_ASSERT_EQUALS(5_bytes, elem.voidElements.back().skipBytes);
    TEST_ASSERT(readMember);
    TEST_ASSERT_EQUALS(Void::ID, readMember());
    TEST_ASSERT_EQUALS(8_bytes, elem.voidElements.back().skipBytes);
    TEST_ASSERT(readMember);
    TEST_ASSERT_EQUALS(Void::ID, readMember());
    TEST_ASSERT_EQUALS(7_bytes, elem.voidElements.back().skipBytes);
    TEST_ASSERT(readMember);
    TEST_ASSERT_EQUALS(Void::ID, readMember());
    TEST_ASSERT_EQUALS(8_bytes, elem.voidElements.back().skipBytes);

    TEST_ASSERT(readMember);
    TEST_ASSERT_EQUALS(ElementId{}, readMember());
    TEST_ASSERT_FALSE(readMember);
    TEST_ASSERT_FALSE(reader.hasMoreBytes());
    TEST_ASSERT_EQUALS(6U, elem.voidElements.size());
  }

  void testChunkedReadPrematureEos() {
    const auto DATA = toBytes({0xF0, 0xFF, 0xBF, 0x84, 0x1B, 0xE4, 0xC4, 0x81, 0xEC, 0x88, 0x00, 0x00, 0x00});

    ReadOptions options{};
    options.validateCRC32 = false;

    // Terminated by end of stream
    bml::BitReader reader{std::span{DATA}};
    TestDynamicElement elem{};
    auto readMember = elem.readChunked(reader, options);

    TEST_ASSERT(readMember);
    TEST_ASSERT_EQUALS(CRC32::ID, readMember());
    TEST_ASSERT(elem.crc32);
    TEST_ASSERT_EQUALS(0x81C4E41BU, elem.crc32->get());

    TEST_THROWS(readMember(), EndOfStreamError);
    TEST_ASSERT_FALSE(readMember);
  }

  void testChunkedReadPartial() {
    const auto DATA = toBytes({0xF0, 0xFF, 0xBF, 0x84, 0xFF, 0xFF, 0xFF, 0xFF, 0xEC, 0x83, 0x00, 0x00, 0x00, 0xEC,
                               0x84, 0x00, 0x00, 0x00, 0x00, 0xEC, 0x85, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEC, 0x88,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEC, 0x87, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0xEC, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});

    ReadOptions options{};
    options.validateCRC32 = true;

    // Terminated by end of stream
    bml::BitReader reader{std::span{DATA}};
    TestDynamicElement elem{};
    auto readMember = elem.readChunked(reader, options);

    TEST_ASSERT(readMember);
    TEST_ASSERT_EQUALS(CRC32::ID, readMember());
    TEST_ASSERT(elem.crc32);
    TEST_ASSERT_EQUALS(0xFFFFFFFFU, elem.crc32->get());

    TEST_ASSERT(readMember);
    TEST_ASSERT_EQUALS(Void::ID, readMember());
    TEST_ASSERT_EQUALS(3_bytes, elem.voidElements.back().skipBytes);
    TEST_ASSERT(readMember);
    TEST_ASSERT_EQUALS(Void::ID, readMember());
    TEST_ASSERT_EQUALS(4_bytes, elem.voidElements.back().skipBytes);
    TEST_ASSERT(readMember);
    // stop reading before reaching the end, chunked reader should be cleaned up properly
    // also does not throw on invalid CRC, since we never reach the check
  }

  void testChunkedReadUnsymmetricUsage() {
    // Check non-symmetric usage of the chunked reader's bool and call operators
    const auto DATA = toBytes({0xF0, 0xFF, 0xBF, 0x84, 0x1B, 0xE4, 0xC4, 0x81, 0xEC, 0x83, 0x00, 0x00, 0x00, 0xEC,
                               0x84, 0x00, 0x00, 0x00, 0x00, 0xEC, 0x85, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEC, 0x88,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEC, 0x87, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0xEC, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});

    ReadOptions options{};
    options.validateCRC32 = true;

    bml::BitReader reader{std::span{DATA}};
    TestDynamicElement elem{};
    auto readMember = elem.readChunked(reader, options);

    // The bool operator does not read any bits
    TEST_ASSERT_EQUALS(0_bits, reader.position());
    TEST_ASSERT(readMember);
    TEST_ASSERT(readMember);
    TEST_ASSERT(readMember);
    TEST_ASSERT(readMember);
    TEST_ASSERT(readMember);
    TEST_ASSERT(readMember);
    TEST_ASSERT_EQUALS(0_bits, reader.position());

    TEST_ASSERT_EQUALS(CRC32::ID, readMember());
    TEST_ASSERT_EQUALS(0U, elem.voidElements.size());
    TEST_ASSERT_EQUALS(Void::ID, readMember());
    TEST_ASSERT_EQUALS(1U, elem.voidElements.size());
    TEST_ASSERT_EQUALS(Void::ID, readMember());
    TEST_ASSERT_EQUALS(2U, elem.voidElements.size());
    TEST_ASSERT_EQUALS(Void::ID, readMember());
    TEST_ASSERT_EQUALS(3U, elem.voidElements.size());
    TEST_ASSERT_EQUALS(Void::ID, readMember());
    TEST_ASSERT_EQUALS(4U, elem.voidElements.size());
    auto before = reader.position();
    TEST_ASSERT(readMember);
    TEST_ASSERT(readMember);
    TEST_ASSERT_EQUALS(before, reader.position());

    TEST_ASSERT_EQUALS(Void::ID, readMember());
    TEST_ASSERT_EQUALS(5U, elem.voidElements.size());
    TEST_ASSERT_EQUALS(Void::ID, readMember());
    TEST_ASSERT_EQUALS(6U, elem.voidElements.size());
    TEST_ASSERT(readMember);
    TEST_ASSERT_EQUALS(ElementId{}, readMember());
    TEST_ASSERT_FALSE(readMember);
  }

  void testConcurrentChunkedReaders() {
    // Check that we can read (and validate CRCs) from multiple chunked readers in parallel in one thread
    const auto DATA1 = toBytes({0xF0, 0xFF, 0xBF, 0x84, 0x1B, 0xE4, 0xC4, 0x81, 0xEC, 0x83, 0x00, 0x00, 0x00, 0xEC,
                                0x84, 0x00, 0x00, 0x00, 0x00, 0xEC, 0x85, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEC, 0x88,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEC, 0x87, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0xEC, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});

    const auto DATA2 = toBytes({0xF0, 0x98, 0xBF, 0x84, 0xDF, 0x53, 0xCB, 0x06, 0xEC, 0x83, 0x00, 0x00, 0x00,
                                0xEC, 0x84, 0x00, 0x00, 0x00, 0x00, 0xEC, 0x85, 0x00, 0x00, 0x00, 0x00, 0x00});

    bml::BitReader reader1{DATA1};
    bml::BitReader reader2{DATA2};
    bml::ebml::ReadOptions options{};
    options.validateCRC32 = true;

    TestDynamicElement elem1{};
    TestDynamicElement elem2{};

    auto read1 = elem1.readChunked(reader1, options);
    auto read2 = elem2.readChunked(reader2, options);

    while (read1 || read2) {
      if (read1) {
        read1();
      }
      if (read2) {
        read2();
      }
    }

    TEST_ASSERT(elem1.crc32);
    TEST_ASSERT_EQUALS(0x81C4E41BU, elem1.crc32.value());
    TEST_ASSERT_EQUALS(6U, elem1.voidElements.size());

    TEST_ASSERT(elem2.crc32);
    TEST_ASSERT_EQUALS(0x06CB53DFU, elem2.crc32.value());
    TEST_ASSERT_EQUALS(3U, elem2.voidElements.size());
  }

private:
  template <EBMLElement Element, typename T>
  void testValueElement(const Element &expectedElement, const T &expectedValue, const std::vector<uint8_t> &data,
                        std::string_view expectedString, bml::ByteCount expectedSize)
    requires(ValueBitField<Element, T>)
  {
    testValueBitField<Element, T>(expectedElement, expectedValue, data, expectedString, expectedSize);
  }

  template <typename Element, typename T>
  void testValueBitField(const Element &expectedElement, const T &expectedValue, const std::vector<uint8_t> &data,
                         std::string_view expectedString, bml::ByteCount expectedSize)
    requires(ValueBitField<Element, T>)
  {
    Element element{};
    checkParseElement(std::as_bytes(std::span{data}), element);
    TEST_ASSERT_EQUALS(expectedElement, element);
    TEST_ASSERT_EQUALS(expectedValue, element);

    checkWriteElement(std::as_bytes(std::span{data}), expectedElement);
    checkPrintElement(expectedElement, expectedString);
    checkElementSize(expectedElement, expectedSize);
    checkSkipElement<Element>(std::as_bytes(std::span{data}));
    checkCopyElement<Element>(std::as_bytes(std::span{data}));
  }

  template <EBMLElement Element, typename T = decltype(Element::DEFAULT)>
  void testValueElementDefaultValue(const Element &expectedElement, const std::vector<uint8_t> &data,
                                    std::string_view expectedString, bml::ByteCount expectedSize,
                                    T defaultValue = Element::DEFAULT)
    requires(ValueBitField<Element, T>)
  {
    testValueBitFieldDefaultValue<Element, T>(expectedElement, data, expectedString, expectedSize, defaultValue);
  }

  template <typename Element, typename T = decltype(Element::DEFAULT)>
  void testValueBitFieldDefaultValue(const Element &expectedElement, const std::vector<uint8_t> &data,
                                     std::string_view expectedString, bml::ByteCount expectedSize,
                                     T defaultValue = Element::DEFAULT)
    requires(ValueBitField<Element, T>)
  {
    Element element{};
    checkParseElement(std::as_bytes(std::span{data}), element);
    TEST_ASSERT_EQUALS(expectedElement, element);
    TEST_ASSERT_EQUALS(defaultValue, element);

    checkWriteElement(toBytes({}), expectedElement);
    checkPrintElement(expectedElement, expectedString);
    checkElementSize(expectedElement, expectedSize);
    checkSkipElement<Element>(std::as_bytes(std::span{data}));
    checkCopyElement<Element>(std::as_bytes(std::span{data}));
  }
};

class TestCommonElements : public TestElementsBase {
public:
  TestCommonElements() : TestElementsBase("CommonElements") {
    TEST_ADD(TestCommonElements::testCRC32);
    TEST_ADD(TestCommonElements::testVoid);
    TEST_ADD(TestCommonElements::testDocTypeExtension);
    TEST_ADD(TestCommonElements::testEBMLHeader);
  }

  void testCRC32() {
    CRC32 elem{};
    elem = 0x8AC5F5D1;

    testElement(elem, {0xBF, 0x84, 0xD1, 0xF5, 0xC5, 0x8A}, "0x8ac5f5d1");
  }

  void testVoid() {
    Void elem{};
    elem.skipBytes = 86_bytes;

    testElement(elem, {0xEC, 0xD6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                "skipBytes: 86");
  }

  void testDocTypeExtension() {
    DocTypeExtension ext{};
    ext.docTypeExtensionName = "FOO";
    ext.docTypeExtensionVersion = 17;

    testElement(ext, {0x42, 0x81, 0x8A, 0x42, 0x83, 0x83, 0x46, 0x4F, 0x4F, 0x42, 0x84, 0x81, 0x11},
                "docTypeExtensionName: 'FOO'\ndocTypeExtensionVersion: 17");
  }

  void testEBMLHeader() {
    EBMLHeader header{};
    header.crc32 = 0xF0C83F84U;
    header.version = 2U;
    header.readVersion = 1U;
    header.maxIDLength = 4U;
    header.maxSizeLength = 8U;
    header.docType = "matroska";
    header.docTypeVersion = 4U;
    header.docTypeReadVersion = 2U;

    testElement(header, {0x1a, 0x45, 0xdf, 0xa3, 0xa9, 0xBF, 0x84, 0x84, 0x3f, 0xc8, 0xf0, 0x42, 0x86, 0x81, 0x02, 0x42,
                         0xf7, 0x81, 0x01, 0x42, 0xf2, 0x81, 0x04, 0x42, 0xf3, 0x81, 0x08, 0x42, 0x82, 0x88, 0x6d, 0x61,
                         0x74, 0x72, 0x6f, 0x73, 0x6b, 0x61, 0x42, 0x87, 0x81, 0x04, 0x42, 0x85, 0x81, 0x02},
                R"(crc32: 0xf0c83f84
version: 2
readVersion: 1
maxIDLength: 4
maxSizeLength: 8
docType: 'matroska'
docTypeVersion: 4
docTypeReadVersion: 2)");
  }

private:
  template <EBMLElement Element>
  void testElement(const Element &expectedElement, const std::vector<uint8_t> &data,
                   std::string_view expectedYAMLString) {
    Element element{};
    checkParseElement(std::as_bytes(std::span{data}), element);
    TEST_ASSERT_EQUALS(expectedElement, element);

    checkWriteElement(std::as_bytes(std::span{data}), expectedElement);
    checkPrintYAMLElement(expectedElement, expectedYAMLString);
    checkSkipElement<Element>(std::as_bytes(std::span{data}));
    checkCopyElement<Element>(std::as_bytes(std::span{data}));
  }
};

int main(int argc, char **argv) {
  Test::registerSuite(Test::newInstance<TestBaseElements>, "base");
  Test::registerSuite(Test::newInstance<TestCommonElements>, "common");
  registerMkvTestSuites();
  return Test::runSuites(argc, argv);
}
