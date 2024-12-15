#pragma once

#include "sizes.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <memory>
#include <vector>

using namespace bml;

void registerBinaryMapTests();
void registerTypeTests();

enum class EnumType : uint8_t { FOO = 1, BAR = 17, BAZ = 32 };
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

struct NonTrival2 {
public:
  int foo;

  void setBar(float b) { bar = b; }
  float getBar() const { return bar; }

  void setChar(char val) { c = val; }
  char getChar() const { return c; }

  auto operator<=>(const NonTrival2 &other) const noexcept = default;

  friend std::ostream &operator<<(std::ostream &os, const NonTrival2 &val) {
    return os << '{' << val.foo << ", " << val.bar << ", " << val.c << '}';
  }

private:
  float bar;

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

struct POD2 {
  int32_t a;
  char c;
  uint32_t length;
  std::string text;
  std::array<uint8_t, 3> buf;

  void setA(int32_t a) { this->a = a; }
  int32_t getA() const { return a; }
  void setC(char c) { this->c = c; }
  char getC() const { return c; }

  BML_DEFINE_PRINT(POD2, a, c, length, text, buf)

  // These are required for testing
  bool operator==(const POD2 &) const noexcept = default;
};

struct DynamicallySizedDestructurable {
  uint8_t u;
  PrefixString<8> s;

  BML_DEFINE_PRINT(DynamicallySizedDestructurable, u, s)

  // These are required for testing
  bool operator==(const DynamicallySizedDestructurable &) const noexcept = default;
};
static_assert(BitField<DynamicallySizedDestructurable>);

inline std::vector<std::byte> toBytes(std::initializer_list<uint8_t> list) {
  std::vector<std::byte> vec{};
  vec.resize(list.size());
  std::transform(list.begin(), list.end(), vec.begin(), [](uint8_t val) { return std::bit_cast<std::byte>(val); });
  return vec;
}
