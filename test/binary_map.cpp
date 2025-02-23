#include "binary_map.hpp"

#include "test_helper.hpp"

#include "cpptest-main.h"

#include <iostream>

using namespace bml;

namespace bml::detail {

  template <auto Value>
  std::ostream &operator<<(std::ostream &os, const CompileTimeValueHolder<Value> &holder) {
    return os << holder.value().toString();
  }

  template <typename T>
  std::ostream &operator<<(std::ostream &os, const RunTimeValueHolder<T> &holder) {
    return os << holder.value().toString();
  }
} // namespace bml::detail

static_assert(sizeof(detail::CompileTimeValueHolder<1235445435ULL>) == 1);
#ifdef __GNUC__
// MSVC does not heed [[no_unique_address]] attribute,
// see https://devblogs.microsoft.com/cppblog/msvc-cpp20-and-the-std-cpp20-switch/#c++20-[[no_unique_address]]
static_assert(sizeof(mapExpGolombBits<uint32_t>()) == 1);
static_assert(sizeof(assertByteAligned()) == 1);
#endif

class TestMappers : public Test::Suite {
public:
  TestMappers() : Test::Suite("Mappers") {

    TEST_ADD(TestMappers::testUnsignedInteger);
    TEST_ADD(TestMappers::testSignedInteger);
    TEST_ADD(TestMappers::testUnicode);
    TEST_ADD(TestMappers::testEnum);
    TEST_ADD(TestMappers::testCompound);
    TEST_ADD(TestMappers::testMembers);
    TEST_ADD(TestMappers::testErrors);
  }

  void testUnsignedInteger() {
    checkFixedSizeDirectMapper(uint8_t{42}, mapBits<uint8_t>(6_bits), toBytes({0xA8}), 6_bits);
    checkFixedSizeDirectMapper(uint64_t{42}, mapBits<uint64_t, 7_bits>(), toBytes({0x54}), 7_bits);
    checkFixedSizeDirectMapper(uint16_t{17}, mapBits<uint16_t>(), toBytes({0x00, 0x11}), 2_bytes);

    checkFixedSizeDirectMapper(uint8_t{17}, mapBytes<uint8_t>(2_bytes), toBytes({0x00, 0x11}), 2_bytes);
    checkFixedSizeDirectMapper(uint64_t{42}, mapBytes<uint64_t, 2_bytes>(), toBytes({0x00, 0x2A}), 2_bytes);
    checkFixedSizeDirectMapper(uint16_t{17}, mapBytes<uint16_t>(), toBytes({0x00, 0x11}), 2_bytes);

    checkDynamicSizeDirectMapper(uint8_t{107}, mapExpGolombBits<uint8_t>(), toBytes({0x03, 0x60}), 13_bits);
    checkDynamicSizeDirectMapper(
        uintmax_t{0xFFFFFFFFFFFFFFFE}, mapExpGolombBits<uintmax_t>(),
        toBytes({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE}),
        127_bits);

    checkDynamicSizeDirectMapper(uint32_t{1}, mapFibonacciBits<uint32_t>(), toBytes({0xC0}), 2_bits);
    checkDynamicSizeDirectMapper(uint32_t{107}, mapFibonacciBits<uint32_t>(), toBytes({0x14, 0x60}), 11_bits);
    checkDynamicSizeDirectMapper(uint32_t{0xFFFFFFFF}, mapFibonacciBits<uint32_t>(),
                                 toBytes({0x24, 0x88, 0x08, 0xA2, 0xA1, 0x16}), 47_bits);

    checkFixedSizeDirectMapper(uint8_t{42}, mapUncheckedFixedBits(uint8_t{42}, 6_bits), toBytes({0xA8}), 6_bits);
    checkFixedSizeDirectMapper(uint8_t{42}, mapUncheckedFixedBits<uint8_t{42}, 7_bits>(), toBytes({0x54}), 7_bits);
    checkFixedSizeDirectMapper(uint8_t{42}, mapCheckedFixedBits(uint8_t{42}, 6_bits), toBytes({0xA8}), 6_bits);
    checkFixedSizeDirectMapper(uint8_t{42}, mapCheckedFixedBits<uint8_t{42}, 7_bits>(), toBytes({0x54}), 7_bits);

    checkFixedSizeDirectMapper(uint8_t{42}, mapUncheckedFixedBytes(uint8_t{42}, 1_bytes), toBytes({0x2A}), 1_bytes);
    checkFixedSizeDirectMapper(uint8_t{42}, mapUncheckedFixedBytes<uint8_t{42}, 2_bytes>(), toBytes({0x00, 0x2A}),
                               2_bytes);
    checkFixedSizeDirectMapper(uint8_t{42}, mapCheckedFixedBytes(uint8_t{42}, 1_bytes), toBytes({0x2A}), 1_bytes);
    checkFixedSizeDirectMapper(uint8_t{42}, mapCheckedFixedBytes<uint8_t{42}, 2_bytes>(), toBytes({0x00, 0x2A}),
                               2_bytes);
  }

  void testSignedInteger() {
    checkFixedSizeDirectMapper(int8_t{42}, mapBits<int8_t>(6_bits), toBytes({0xA8}), 6_bits);
    checkFixedSizeDirectMapper(int64_t{42}, mapBits<int64_t, 7_bits>(), toBytes({0x54}), 7_bits);
    checkFixedSizeDirectMapper(int16_t{17}, mapBits<int16_t>(), toBytes({0x00, 0x11}), 2_bytes);

    checkFixedSizeDirectMapper(int8_t{17}, mapBytes<int8_t>(2_bytes), toBytes({0x00, 0x11}), 2_bytes);
    checkFixedSizeDirectMapper(int64_t{42}, mapBytes<int64_t, 2_bytes>(), toBytes({0x00, 0x2A}), 2_bytes);
    checkFixedSizeDirectMapper(int16_t{17}, mapBytes<int16_t>(), toBytes({0x00, 0x11}), 2_bytes);

    checkDynamicSizeDirectMapper(intmax_t{0}, mapExpGolombBits<intmax_t>(), toBytes({0x80}), 1_bits);
    checkDynamicSizeDirectMapper(intmax_t{107}, mapExpGolombBits<intmax_t>(), toBytes({0x01, 0xAC}), 15_bits);
    checkDynamicSizeDirectMapper(intmax_t{-107}, mapExpGolombBits<intmax_t>(), toBytes({0x01, 0xAE}), 15_bits);
    checkDynamicSizeDirectMapper(
        intmax_t{0x3FFFFFFFFFFFFFFF}, mapExpGolombBits<intmax_t>(),
        toBytes({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0}),
        125_bits);
    checkDynamicSizeDirectMapper(
        intmax_t{-0x3FFFFFFFFFFFFFFF}, mapExpGolombBits<intmax_t>(),
        toBytes({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8}),
        125_bits);

    checkDynamicSizeDirectMapper(int32_t{1}, mapFibonacciBits<int32_t>(), toBytes({0xC0}), 2_bits);
    checkDynamicSizeDirectMapper(int32_t{-1}, mapFibonacciBits<int32_t>(), toBytes({0x60}), 3_bits);
    checkDynamicSizeDirectMapper(int32_t{107}, mapFibonacciBits<int32_t>(), toBytes({0x0A, 0x30}), 12_bits);
    checkDynamicSizeDirectMapper(int32_t{-107}, mapFibonacciBits<int32_t>(), toBytes({0xA0, 0x98}), 13_bits);
    checkDynamicSizeDirectMapper(int32_t{0x3FFFFFFF}, mapFibonacciBits<int32_t>(),
                                 toBytes({0x8A, 0x81, 0x02, 0x29, 0x54, 0x0C}), 46_bits);
    checkDynamicSizeDirectMapper(int32_t{-0x3FFFFFFF}, mapFibonacciBits<int32_t>(),
                                 toBytes({0x20, 0x29, 0x52, 0x81, 0x01, 0x58}), 45_bits);

    checkFixedSizeDirectMapper(int8_t{42}, mapUncheckedFixedBits(int8_t{42}, 6_bits), toBytes({0xA8}), 6_bits);
    checkFixedSizeDirectMapper(int8_t{42}, mapUncheckedFixedBits<int8_t{42}, 7_bits>(), toBytes({0x54}), 7_bits);
    checkFixedSizeDirectMapper(int8_t{42}, mapCheckedFixedBits(int8_t{42}, 6_bits), toBytes({0xA8}), 6_bits);
    checkFixedSizeDirectMapper(int8_t{42}, mapCheckedFixedBits<int8_t{42}, 7_bits>(), toBytes({0x54}), 7_bits);

    checkFixedSizeDirectMapper(int8_t{42}, mapUncheckedFixedBytes(int8_t{42}, 1_bytes), toBytes({0x2A}), 1_bytes);
    checkFixedSizeDirectMapper(int8_t{42}, mapUncheckedFixedBytes<int8_t{42}, 2_bytes>(), toBytes({0x00, 0x2A}),
                               2_bytes);
    checkFixedSizeDirectMapper(int8_t{42}, mapCheckedFixedBytes(int8_t{42}, 1_bytes), toBytes({0x2A}), 1_bytes);
    checkFixedSizeDirectMapper(int8_t{42}, mapCheckedFixedBytes<int8_t{42}, 2_bytes>(), toBytes({0x00, 0x2A}), 2_bytes);
  }

  void testUnicode() {
    checkDynamicSizeDirectMapper(U'\0', mapUtf8Bytes<>(), toBytes({0x00}), 1_bytes);
    checkDynamicSizeDirectMapper(U'A', mapUtf8Bytes<>(), toBytes({0x41}), 1_bytes);
    checkDynamicSizeDirectMapper(U'£', mapUtf8Bytes<>(), toBytes({0xC2, 0xA3}), 2_bytes);
    checkDynamicSizeDirectMapper(U'÷', mapUtf8Bytes<>(), toBytes({0xC3, 0xB7}), 2_bytes);
    checkDynamicSizeDirectMapper(U'ᴀ', mapUtf8Bytes<>(), toBytes({0xE1, 0xB4, 0x80}), 3_bytes); // small capital A
    checkDynamicSizeDirectMapper(U'₤', mapUtf8Bytes<>(), toBytes({0xE2, 0x82, 0xA4}), 3_bytes); // Lira
    checkDynamicSizeDirectMapper(U'𐞃', mapUtf8Bytes<>(), toBytes({0xF0, 0x90, 0x9E, 0x83}), 4_bytes);
    checkDynamicSizeDirectMapper(U'🚆', mapUtf8Bytes<>(), toBytes({0xF0, 0x9F, 0x9A, 0x86}), 4_bytes); // train
    checkDynamicSizeDirectMapper(U'🤍', mapUtf8Bytes<>(), toBytes({0xF0, 0x9F, 0xA4, 0x8D}), 4_bytes); // white heart
  }

  void testEnum() {
    checkFixedSizeDirectMapper(EnumType::BAZ, mapBits<EnumType>(6_bits), toBytes({0x80}), 6_bits);
    checkFixedSizeDirectMapper(EnumType::BAZ, mapBits<EnumType, 7_bits>(), toBytes({0x40}), 7_bits);
    checkFixedSizeDirectMapper(LargeEnum::BAZ, mapBits<LargeEnum>(), toBytes({0x00, 0x00, 0x00, 0x20}), 4_bytes);

    checkFixedSizeDirectMapper(EnumType::BAR, mapBytes<EnumType>(2_bytes), toBytes({0x00, 0x11}), 2_bytes);
    checkFixedSizeDirectMapper(EnumType::BAR, mapBytes<EnumType, 2_bytes>(), toBytes({0x00, 0x11}), 2_bytes);
    checkFixedSizeDirectMapper(LargeEnum::BAR, mapBytes<LargeEnum>(), toBytes({0x00, 0x00, 0x00, 0x11}), 4_bytes);

    checkFixedSizeDirectMapper(EnumType::BAZ, mapUncheckedFixedBits(EnumType::BAZ, 6_bits), toBytes({0x80}), 6_bits);
    checkFixedSizeDirectMapper(EnumType::BAZ, mapUncheckedFixedBits<EnumType::BAZ, 7_bits>(), toBytes({0x40}), 7_bits);
    checkFixedSizeDirectMapper(EnumType::BAZ, mapCheckedFixedBits(EnumType::BAZ, 6_bits), toBytes({0x80}), 6_bits);
    checkFixedSizeDirectMapper(EnumType::BAZ, mapCheckedFixedBits<EnumType::BAZ, 7_bits>(), toBytes({0x40}), 7_bits);

    checkFixedSizeDirectMapper(EnumType::BAR, mapUncheckedFixedBytes(EnumType::BAR, 1_bytes), toBytes({0x11}), 1_bytes);
    checkFixedSizeDirectMapper(EnumType::BAR, mapUncheckedFixedBytes<EnumType::BAR, 2_bytes>(), toBytes({0x00, 0x11}),
                               2_bytes);
    checkFixedSizeDirectMapper(EnumType::BAR, mapCheckedFixedBytes(EnumType::BAR, 1_bytes), toBytes({0x11}), 1_bytes);
    checkFixedSizeDirectMapper(EnumType::BAR, mapCheckedFixedBytes<EnumType::BAR, 2_bytes>(), toBytes({0x00, 0x11}),
                               2_bytes);
  }

  void testCompound() {
    NonTrival2 value{};
    value.foo = 17;
    value.setBar(3.0F);
    value.setChar('A');

    auto mapNonTrivial = mapCompound<NonTrival2>(
        mapMemberBits(&NonTrival2::foo),
        mapMemberProperty<&NonTrival2::getBar, &NonTrival2::setBar, mapBits<uint8_t, 5_bits>()>(),
        mapMemberProperty<7_bits>(&NonTrival2::getChar, &NonTrival2::setChar));

    checkFixedSizeDirectMapper(value, mapNonTrivial,
                               toBytes({/* foo */ 0x00, 0x00, 0x00, 0x11, /* bar + char */ 0x1C, 0x10}),
                               4_bytes + 12_bits);
  }

  void testMembers() {
    POD2 pod{};
    pod.a = -107;
    pod.c = ' ';
    pod.length = 3;
    pod.text = "Foo";
    pod.buf = {0x01, 0x02, 0x03};

    checkDynamicSizeMemberMapper(pod, &POD2::a, mapMember(&POD2::a, mapFibonacciBits<int32_t>()), toBytes({0xA0, 0x98}),
                                 13_bits);
    checkFixedSizeMemberMapper(pod, &POD2::c, mapMember<&POD2::c, mapBits<char>()>(), toBytes({0x20}), 1_bytes);
    checkFixedSizeMemberMapper(pod, &POD2::c, mapMemberBits<6_bits>(&POD2::c), toBytes({0x80}), 6_bits);
    checkFixedSizeMemberMapper(pod, &POD2::c, mapMemberBits<&POD2::c, 7_bits>(), toBytes({0x40}), 7_bits);
    checkFixedSizeMemberMapper(pod, &POD2::c, mapMemberBits(&POD2::c), toBytes({0x20}), 1_bytes);
    checkFixedSizeMemberMapper(pod, &POD2::c, mapMemberBits<&POD2::c>(), toBytes({0x20}), 1_bytes);

    checkDynamicSizeMemberMapper(pod, &POD2::a,
                                 mapMemberProperty(&POD2::getA, &POD2::setA, mapFibonacciBits<int32_t>()),
                                 toBytes({0xA0, 0x98}), 13_bits);
    checkDynamicSizeMemberMapper(pod, &POD2::a,
                                 mapMemberProperty<&POD2::getA, &POD2::setA, mapFibonacciBits<int32_t>()>(),
                                 toBytes({0xA0, 0x98}), 13_bits);
    checkFixedSizeMemberMapper(pod, &POD2::c, mapMemberProperty<6_bits>(&POD2::getC, &POD2::setC), toBytes({0x80}),
                               6_bits);
    checkFixedSizeMemberMapper(pod, &POD2::c, mapMemberProperty<&POD2::getC, &POD2::setC, 7_bits>(), toBytes({0x40}),
                               7_bits);
    checkFixedSizeMemberMapper(pod, &POD2::c, mapMemberProperty(&POD2::getC, &POD2::setC), toBytes({0x20}), 1_bytes);
    checkFixedSizeMemberMapper(pod, &POD2::c, mapMemberProperty<&POD2::getC, &POD2::setC>(), toBytes({0x20}), 1_bytes);

    checkDynamicSizeMemberMapper(pod, &POD2::text, mapMemberContainer(&POD2::text, mapBits<char>(), &POD2::length),
                                 toBytes({0x46, 0x6F, 0x6F}), 3_bytes);
    checkDynamicSizeMemberMapper(pod, &POD2::text, mapMemberContainer<&POD2::text, mapBits<char>(), &POD2::length>(),
                                 toBytes({0x46, 0x6F, 0x6F}), 3_bytes);

    checkFixedSizeMemberMapper(pod, &POD2::buf, mapMemberArray(&POD2::buf, mapBits<char>(2_bits)),
                               toBytes({0b01'10'11'00}), 6_bits);
    checkFixedSizeMemberMapper(pod, &POD2::buf, mapMemberArray<&POD2::buf, mapBits<char>(2_bits)>(),
                               toBytes({0b01'10'11'00}), 6_bits);
  }

  void testErrors() {

    // Invalid fixed value error
    {
      auto data = toBytes({0x11});
      BitReader reader{data};
      TEST_THROWS((mapCheckedFixedBits<uint8_t{42}, 7_bits, std::logic_error>())(reader), std::logic_error);
    }

    // Invalid alignment error
    {
      auto data = toBytes({0x11, 0x01, 0x12});
      BitReader reader{data};
      std::ignore = reader.read();
      TEST_THROWS(mapBytes<uint16_t>()(reader), std::invalid_argument);

      std::vector<std::byte> outData(data.size());
      BitWriter writer{std::span{outData}};
      writer.write(true);
      TEST_THROWS(mapBytes<uint16_t>()(writer, 16), std::invalid_argument);
    }

    {
      auto data = toBytes({0x11, 0x01, 0x12});
      BitReader reader{data};
      std::ignore = reader.read();
      TEST_THROWS(assertByteAligned()(reader), std::invalid_argument);

      std::vector<std::byte> outData(data.size());
      BitWriter writer{std::span{outData}};
      writer.write(true);
      TEST_THROWS(assertByteAligned()(writer), std::invalid_argument);

      TEST_ASSERT_EQUALS(0_bits, assertByteAligned().fixedSize());
    }
  }

private:
  template <DirectMapper Mapper>
  void checkDirectMapper(const typename Mapper::type &expectedValue, const Mapper &map, std::span<const std::byte> data,
                         BitCount numBits = ByteCount{sizeof(typename Mapper::type)}) {
    // Read
    {
      BitReader reader{data};
      typename Mapper::type value{};
      TEST_THROWS_NOTHING(value = map(reader));
      TEST_ASSERT_EQUALS(numBits, reader.position());
      TEST_ASSERT_EQUALS(expectedValue, value);
    }

    // Write
    {
      std::vector<std::byte> outData(data.size());
      BitWriter writer{std::span{outData}};
      TEST_THROWS_NOTHING(map(writer, expectedValue));
      TEST_ASSERT_EQUALS(numBits, writer.position());
      writer.flush();
      TEST_ASSERT_EQUALS(data, outData);
    }
  }

  template <MemberMapper Mapper>
  void checkMemberMapper(const typename Mapper::object_type &expectedObject, const auto &member, const Mapper &map,
                         std::span<const std::byte> data,
                         BitCount numBits = ByteCount{sizeof(typename Mapper::member_type)}) {
    // Read
    {
      BitReader reader{data};
      typename Mapper::object_type object = expectedObject;
      TEST_THROWS_NOTHING(map(reader, object));
      TEST_ASSERT_EQUALS(numBits, reader.position());
      TEST_ASSERT_EQUALS(expectedObject.*member, object.*member);
    }

    // Write
    {
      std::vector<std::byte> outData(data.size());
      BitWriter writer{std::span{outData}};
      TEST_THROWS_NOTHING(map(writer, expectedObject));
      TEST_ASSERT_EQUALS(numBits, writer.position());
      writer.flush();
      TEST_ASSERT_EQUALS(data, outData);
    }
  }

  template <DirectMapper Mapper>
  void checkDynamicSizeDirectMapper(const typename Mapper::type &expectedValue, const Mapper &map,
                                    std::span<const std::byte> data,
                                    BitCount numBits = ByteCount{sizeof(typename Mapper::type)})
    requires(!FixedSizeMapper<Mapper>)
  {
    checkDirectMapper(expectedValue, map, data, numBits);
  }

  template <DirectMapper Mapper>
  void checkFixedSizeDirectMapper(const typename Mapper::type &expectedValue, const Mapper &map,
                                  std::span<const std::byte> data,
                                  BitCount numBits = ByteCount{sizeof(typename Mapper::type)})
    requires(FixedSizeMapper<Mapper>)
  {
    TEST_ASSERT_EQUALS(numBits, map.fixedSize());
    checkDirectMapper(expectedValue, map, data, numBits);
  }

  template <MemberMapper Mapper>
  void checkDynamicSizeMemberMapper(const typename Mapper::object_type &expectedObject, const auto &member,
                                    const Mapper &map, std::span<const std::byte> data,
                                    BitCount numBits = ByteCount{sizeof(typename Mapper::type)})
    requires(!FixedSizeMapper<Mapper>)
  {
    checkMemberMapper(expectedObject, member, map, data, numBits);
  }

  template <MemberMapper Mapper>
  void checkFixedSizeMemberMapper(const typename Mapper::object_type &expectedObject, const auto &member,
                                  const Mapper &map, std::span<const std::byte> data,
                                  BitCount numBits = ByteCount{sizeof(typename Mapper::type)})
    requires(FixedSizeMapper<Mapper>)
  {
    TEST_ASSERT_EQUALS(numBits, map.fixedSize());
    checkMemberMapper(expectedObject, member, map, data, numBits);
  }
};

void registerBinaryMapTests() { Test::registerSuite(Test::newInstance<TestMappers>, "mappers"); }
