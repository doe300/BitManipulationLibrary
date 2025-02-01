#pragma once

#include "ebml.hpp"

#include "cpptest.h"

#include <vector>

struct TestElementsBase : public Test::Suite {
protected:
  TestElementsBase(const std::string &name) : Test::Suite(name) {}

  template <bml::ebml::EBMLElement T, bool ReadContent = false>
  void checkParseElement(std::span<const std::byte> data, T &outElement) {
    bml::BitReader reader{data};
    bml::ebml::ReadOptions options{};
    options.validateCRC32 = true;
    options.readMediaData = ReadContent;
    outElement.read(reader, options);
    TEST_THROWS_NOTHING(reader.assertAlignment(bml::ByteCount{1}));
    TEST_ASSERT_FALSE(reader.hasMoreBytes());
  }

  template <typename T>
  void checkWriteElement(std::span<const std::byte> inputData, const T &element) {
    std::vector<std::byte> result{};
    bml::BitWriter writer{[&](std::byte b) {
      result.emplace_back(b);
      return true;
    }};
    bml::write(writer, element);

    TEST_ASSERT_EQUALS(inputData, result);
  }

  template <typename T>
  void checkPrintElement(const T &val, std::string_view expectedVal)
    requires(bml::Printable<T>)
  {
    std::stringstream ss{};
    ss << bml::printView(val);
    TEST_ASSERT_EQUALS(expectedVal, ss.str());
  }

  template <typename T>
  void checkPrintYAMLElement(const T &val, std::string_view expectedVal)
    requires(bml::Printable<T>)
  {
    std::stringstream ss{};
    bml::yaml::Options opts{};
    opts.hideEmpty = true;
    bml::yaml::print(ss, opts, val);
    TEST_ASSERT_EQUALS(expectedVal, ss.str());
  }

  template <typename T>
  void checkElementSize(const T &val, bml::ByteCount expectedSize)
    requires(bml::Sized<T>)
  {
    TEST_ASSERT(bml::minNumBits<T>() <= expectedSize);
    TEST_ASSERT_EQUALS(expectedSize, bml::numBits(val));
    TEST_ASSERT(bml::maxNumBits<T>() >= expectedSize);
  }

  template <typename T>
  void checkSkipElement(std::span<const std::byte> data, bml::ByteCount numBytes = {}) {
    bml::BitReader reader{data};
    TEST_THROWS_NOTHING(bml::skip<T>(reader));
    if (!numBytes) {
      TEST_ASSERT(!reader.hasMoreBytes());
    } else {
      TEST_ASSERT_EQUALS(numBytes, reader.position());
    }
  }

  template <typename T>
  void checkCopyElement(std::span<const std::byte> data, bml::ByteCount numBytes = {}) {
    bml::BitReader reader{data};
    std::vector<std::byte> outputData(data.size());
    bml::BitWriter writer{outputData};
    TEST_THROWS_NOTHING(bml::copy<T>(reader, writer));
    TEST_ASSERT_EQUALS(reader.position(), writer.position());
    if (!numBytes) {
      TEST_ASSERT(!reader.hasMoreBytes());
    } else {
      TEST_ASSERT_EQUALS(numBytes, reader.position());
      TEST_ASSERT_EQUALS(numBytes, writer.position());
    }
    TEST_ASSERT_EQUALS(data, outputData);
  }

  static std::vector<std::byte> toBytes(std::initializer_list<uint8_t> list) {
    std::vector<std::byte> result(list.size());
    std::transform(list.begin(), list.end(), result.begin(), [](uint8_t b) { return std::bit_cast<std::byte>(b); });
    return result;
  }
};

void registerMkvTestSuites();
