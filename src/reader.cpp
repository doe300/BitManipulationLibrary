
#include "reader.hpp"

#include "common.hpp"
#include "errors.hpp"
#include "helper.hpp"

#include <algorithm>
#include <stdexcept>

namespace bml {
  class BitReader::ReaderImpl {
  public:
    ReaderImpl() = default;
    ReaderImpl(const ReaderImpl &) = delete;
    ReaderImpl(ReaderImpl &&) noexcept = delete;
    virtual ~ReaderImpl() noexcept = default;

    ReaderImpl &operator=(const ReaderImpl &) = delete;
    ReaderImpl &operator=(ReaderImpl &&) noexcept = delete;

    BitCount position() const noexcept { return sourceBytesRead - cacheSize; }

    bool hasMoreBytes() noexcept {
      makeAvailable(BYTE_SIZE, false);
      return cacheSize >= BYTE_SIZE;
    }

    bool read() {
      makeAvailable(1_bits);
      auto bit = (cache >> (CACHE_SIZE - 1_bits)) & 1u;
      --cacheSize;
      cache <<= 1u;
      return bit;
    }

    std::optional<std::uintmax_t> peek(BitCount numBits) {
      makeAvailable(numBits, false);
      if (cacheSize < numBits) {
        return {};
      }
      Cache tmp{cache, cacheSize};
      return readFromCache(tmp, numBits);
    }

    std::uintmax_t read(BitCount numBits) {
      if (cacheSize && numBits > cacheSize && (numBits + cacheSize) > (CACHE_SIZE - BYTE_SIZE)) {
        // if we fill a non-empty cache with too many bits, the last byte might no longer fit (e.g. when having 1 bit in
        // cache and loading 64 bits from the source), since only full bytes can be loaded. In this case split the
        // reading.
        auto upper = read(numBits / 2);
        auto lower = read(numBits - (numBits / 2));
        return upper << (numBits - (numBits / 2)) | lower;
      }
      makeAvailable(numBits);
      Cache tmp{cache, cacheSize};
      auto bits = readFromCache(tmp, numBits);
      cache = tmp.value;
      cacheSize = tmp.size;
      return bits;
    }

    std::byte readByte() {
      if (cacheSize) {
        // If we have something cached, read from cache
        return static_cast<std::byte>(read(1_bytes));
      }
      // Otherwise directly read next byte from source
      std::byte byte{};
      if (!extractSourceByte(byte)) [[unlikely]] {
        throwOnEndOfStream();
      }
      ++sourceBytesRead;
      return byte;
    }

    void readBytesInto(std::span<std::byte> outBytes) {
      while (cacheSize && !outBytes.empty()) {
        outBytes[0] = static_cast<std::byte>(read(1_bytes));
        outBytes = outBytes.subspan<1>();
      }
      if (extractSourceBytes(outBytes)) [[likely]] {
        sourceBytesRead += outBytes.size() * 1_bytes;
      } else {
        throwOnEndOfStream();
      }
    }

    ByteCount readBytesUntil(std::span<const std::byte> pattern, std::vector<std::byte> *result) {
      if (!hasMoreBytes() || pattern.empty()) [[unlikely]] {
        return {};
      }

      auto numBytes = 0_bytes;
      // 1. empty cache until we are sure source does not start with pattern
      auto number = toNumber(pattern);
      ByteCount patternSize{pattern.size()};
      while (cacheSize) {
        Cache tmp{cache, cacheSize};
        if (static_cast<std::byte>(readFromCache(tmp, 1_bytes)) == pattern.front() && peek(patternSize) == number) {
          // the first check is to guard to not always fill in the cache with peek(), otherwise it never gets empty
          return numBytes;
        } else [[likely]] {
          // reads 1 byte from cache
          auto b = readByte();
          ++numBytes;
          if (result) {
            result->push_back(b);
          }
        }
      }
      // 2. read remainder directly from the underlying source
      auto additionalBytes = extractSourceBytesUntil(pattern, result);
      sourceBytesRead += additionalBytes;
      return numBytes + additionalBytes;
    }

    void skip(BitCount numBits) {
      auto numBitsSkipped = 0_bits;

      // 1. Read the remainder of the cache
      if (cacheSize) {
        const BitCount numBitsToRead = std::min(numBits - numBitsSkipped, cacheSize);
        read(numBitsToRead);
        numBitsSkipped += numBitsToRead;
      }

      // 2. Skip any remaining full bytes directly in source
      if (auto numBytes = ByteCount{(numBits - numBitsSkipped) / 1_bytes}) {
        if (!skipSourceBytes(numBytes)) [[unlikely]] {
          throw EndOfStreamError("Cannot skip more bytes, end of input reached");
        }
        numBitsSkipped += numBytes;
        sourceBytesRead += numBytes;
      }

      // 3. Skip any remaining bits
      while (numBitsSkipped < numBits) {
        const BitCount numBitsToRead = std::min(numBits - numBitsSkipped, CACHE_SIZE);
        read(numBitsToRead);
        numBitsSkipped += numBitsToRead;
      }
    }

    /**
     * Reads until the uppermost bit in the cache is set and return the number of leading zero bits.
     */
    BitCount readLeadingZeroes() {
      BitCount numBits{};
      while (cache == 0) {
        if (cacheSize) {
          numBits += cacheSize;
          read(cacheSize);
        }
        makeAvailable(BYTE_SIZE);
      }
      const BitCount numRemainingBits{static_cast<uint32_t>(std::countl_zero(cache))};
      read(numRemainingBits);
      numBits += numRemainingBits;
      return numBits;
    }

    /**
     * Reads until two consecutive one bits are set and return the value with then number of bits read.
     */
    EncodedValue<uint64_t> readUntilTwoOnes() {
      EncodedValue<uint64_t> result{};
      while (!(cache & (cache >> 1U))) {
        // as long as there are not two consecutive 1 bits, add all bits
        if (cacheSize) {
          result.numBits += cacheSize - 1_bits;
          result.value <<= cacheSize - 1_bits;
          result.value |= read(cacheSize - 1_bits);
        }
        makeAvailable(BYTE_SIZE);
      }
      bool lastBitSet = false;
      bool nextBitSet = false;
      while (true) {
        nextBitSet = read();
        result.numBits += 1_bits;
        result.value <<= 1_bits;
        result.value |= nextBitSet ? 1U : 0U;
        if (nextBitSet && lastBitSet) {
          break;
        }
        lastBitSet = nextBitSet;
      }
      return result;
    }

  protected:
    void makeAvailable(BitCount numBits, bool throwOnEos = true) {
      Cache tmp{cache, cacheSize};
      sourceBytesRead += fillCache(
          tmp, numBits, [this](std::byte &nextByte) { return extractSourceByte(nextByte); },
          throwOnEos ? throwOnEndOfStream : throwNoError);
      cache = tmp.value;
      cacheSize = tmp.size;
    }

    [[nodiscard]] virtual bool extractSourceByte(std::byte &out) { return extractSourceBytes(std::span{&out, 1}); }
    [[nodiscard]] virtual bool extractSourceBytes(std::span<std::byte> outBytes) = 0;

    virtual ByteCount extractSourceBytesUntil(std::span<const std::byte> pattern, std::vector<std::byte> *result) {
      auto numBytes = 0_bytes;
      std::vector<std::byte> candidate{};
      candidate.reserve(pattern.size());
      std::byte byte{};
      while (extractSourceByte(byte)) [[likely]] {
        if (candidate.empty() && byte != pattern.front()) [[likely]] {
          ++numBytes;
          if (result) {
            result->push_back(byte);
          }
          continue;
        }
        // might be the part of the pattern
        candidate.push_back(byte);
        if (std::equal(candidate.begin(), candidate.end(), pattern.begin())) {
          // candidate still matches so far
          if (candidate.size() == pattern.size()) {
            // pattern found, add to cache and return
            auto tmp = setCache(candidate);
            cache = tmp.value;
            cacheSize = tmp.size;
            sourceBytesRead += ByteCount{candidate.size()};
            candidate.clear();
            break;
          }
          continue;
        }
        // candidate does not match anymore
        // move from the front to the result until the candidate is either empty or matches again (partially)
        auto it = candidate.begin();
        do {
          ++numBytes;
          if (result) {
            result->push_back(*it);
          }
          ++it;
        } while (it != candidate.end() && !std::equal(it, candidate.end(), pattern.begin()));
        candidate.erase(candidate.begin(), it);
      }
      // end of stream, add remaining
      numBytes += ByteCount{candidate.size()};
      if (result) {
        result->insert(result->end(), candidate.begin(), candidate.end());
      }
      return numBytes;
    }

    [[nodiscard]] virtual bool skipSourceBytes(ByteCount numBytes) = 0;

  private:
    static void throwNoError() noexcept {}
    [[noreturn]] static void throwOnEndOfStream() {
      throw EndOfStreamError("Cannot read more bytes, end of input reached");
    }

    static uintmax_t toNumber(std::span<const std::byte> pattern) {
      auto cache = setCache(pattern);
      return readFromCache(cache, ByteCount{pattern.size()});
    }

    static Cache setCache(std::span<const std::byte> bytes) {
      Cache cache{};
      fillCache(
          cache, ByteCount{bytes.size()},
          [bytes](std::byte &out) mutable {
            if (bytes.empty()) {
              return false;
            }
            out = bytes[0];
            bytes = bytes.subspan<1>();
            return true;
          },
          +[]() {});
      return cache;
    }

  private:
    ByteCount sourceBytesRead;
    BitCount cacheSize;
    /** is always left-adjusted to read the highest bits first */
    std::uintmax_t cache = 0;
  };

  struct ByteGeneratorImpl final : BitReader::ReaderImpl {
    explicit ByteGeneratorImpl(BitReader::ByteGenerator &&generator) : generateByte(std::move(generator)) {}

    bool extractSourceByte(std::byte &out) override { return generateByte(out); }

    bool extractSourceBytes(std::span<std::byte> outBytes) override {
      for (auto &b : outBytes) {
        if (!generateByte(b)) {
          return false;
        }
      }
      return true;
    }

    bool skipSourceBytes(ByteCount numBytes) override {
      std::byte dummy{};
      for (auto b = 1_bytes; b <= numBytes; ++b) {
        if (!generateByte(dummy)) {
          return false;
        }
      }
      return true;
    }

    BitReader::ByteGenerator generateByte;
  };

  struct ByteReadRangeImpl final : BitReader::ReaderImpl {
    explicit ByteReadRangeImpl(BitReader::ByteRange range) : sourceRange(range) {}

    bool extractSourceBytes(std::span<std::byte> outBytes) override {
      if (outBytes.size() > sourceRange.size()) {
        return false;
      }
      std::copy_n(sourceRange.begin(), outBytes.size(), outBytes.begin());
      sourceRange = sourceRange.subspan(outBytes.size());
      return true;
    }

    ByteCount extractSourceBytesUntil(std::span<const std::byte> pattern, std::vector<std::byte> *result) override {
      auto end = std::search(sourceRange.begin(), sourceRange.end(), pattern.begin(), pattern.end());
      auto numBytes = ByteCount{static_cast<std::size_t>(std::distance(sourceRange.begin(), end))};
      if (result) {
        result->insert(result->end(), sourceRange.begin(), end);
      }
      sourceRange = BitReader::ByteRange{end, sourceRange.end()};
      return numBytes;
    }

    bool skipSourceBytes(ByteCount numBytes) override {
      if (numBytes > ByteCount{sourceRange.size()}) {
        return false;
      }
      sourceRange = sourceRange.subspan(numBytes.value());
      return true;
    }

    BitReader::ByteRange sourceRange;
  };

  struct ByteInputStreamImpl final : BitReader::ReaderImpl {
    explicit ByteInputStreamImpl(std::istream &is) : input(is) { input.exceptions(std::ios::badbit); }

    bool extractSourceByte(std::byte &out) override {
      if (!input.eof()) {
        input.read(reinterpret_cast<char *>(&out), 1);
        return static_cast<bool>(input);
      }
      return false;
    }

    bool extractSourceBytes(std::span<std::byte> outBytes) override {
      if (!input.eof()) {
        input.read(reinterpret_cast<char *>(outBytes.data()), static_cast<std::streamsize>(outBytes.size()));
        return static_cast<bool>(input);
      }
      return false;
    }

    bool skipSourceBytes(ByteCount numBytes) override {
      input.ignore(static_cast<std::streamsize>(numBytes.value()));
      return static_cast<bool>(input);
    }

    std::istream &input;
  };

  BitReader::BitReader() noexcept = default;
  BitReader::BitReader(ByteGenerator &&generator) : impl(std::make_unique<ByteGeneratorImpl>(std::move(generator))) {}
  BitReader::BitReader(ByteRange range) : impl(std::make_unique<ByteReadRangeImpl>(range)) {}
  BitReader::BitReader(std::istream &is) : impl(std::make_unique<ByteInputStreamImpl>(is)) {}
  BitReader::BitReader(BitReader &&other) noexcept : impl(std::move(other.impl)) { other.impl.reset(); }
  BitReader::~BitReader() noexcept = default;

  BitReader &BitReader::operator=(BitReader &&other) noexcept {
    std::swap(impl, other.impl);
    return *this;
  }

  static BitReader::ReaderImpl &assertImpl(std::unique_ptr<BitReader::ReaderImpl> &ptr) noexcept(false) {
    if (ptr) [[likely]] {
      return *ptr;
    }
    throw std::runtime_error{"Cannot read from empty BitReader instance"};
  }

  BitCount BitReader::position() const noexcept { return impl ? impl->position() : 0_bits; }
  bool BitReader::hasMoreBytes() noexcept { return impl ? impl->hasMoreBytes() : false; }

  BitCount BitReader::skipToAligment(BitCount bitAlignment) {
    auto pos = assertImpl(impl).position();
    if (pos % bitAlignment) {
      auto numBits = bitAlignment - position() % bitAlignment;
      skip(numBits);
      return numBits;
    }
    return 0_bits;
  }

  void BitReader::assertAlignment(BitCount bitAlignment) {
    if (skipToAligment(bitAlignment)) [[unlikely]] {
      throw std::invalid_argument("Input bit stream is not properly aligned");
    }
  }

  bool BitReader::read() { return assertImpl(impl).read(); }

  std::optional<std::uintmax_t> BitReader::peek(BitCount numBits) {
    if (!numBits && impl) [[unlikely]] {
      return 0U;
    }
    return assertImpl(impl).peek(numBits);
  }

  std::uintmax_t BitReader::read(BitCount numBits) {
    if (!numBits && impl) [[unlikely]] {
      return 0U;
    }
    return assertImpl(impl).read(numBits);
  }

  std::uintmax_t BitReader::readBytes(ByteCount numBytes) {
    assertAlignment(BYTE_SIZE);
    if (numBytes == 1_bytes) {
      return static_cast<std::uintmax_t>(assertImpl(impl).readByte());
    }
    return read(numBytes);
  }

  std::byte BitReader::readByte() { return assertImpl(impl).readByte(); }

  void BitReader::readBytesInto(std::span<std::byte> outBytes) {
    if (outBytes.empty() && impl) [[unlikely]] {
      return;
    }

    assertAlignment(BYTE_SIZE);
    assertImpl(impl).readBytesInto(outBytes);
  }

  std::vector<std::byte> BitReader::readBytesUntil(std::span<const std::byte> pattern) {
    assertAlignment(BYTE_SIZE);
    std::vector<std::byte> result{};
    assertImpl(impl).readBytesUntil(pattern, &result);
    return result;
  }

  std::uintmax_t BitReader::readExpGolomb() {
    auto exponent = assertImpl(impl).readLeadingZeroes();
    return decodeExpGolomb(read(exponent + 1_bits /* marker 1-bit */));
  }

  std::intmax_t BitReader::readSignedExpGolomb() {
    auto exponent = assertImpl(impl).readLeadingZeroes();
    return decodeSignedExpGolomb(read(exponent + 1_bits /* marker 1-bit */));
  }

  char32_t BitReader::readUtf8CodePoint() {
    auto b = peek(1_bytes);
    if (b && (*b & 0x80U) == 0x00U) {
      // simple 7-bit ASCII
      return read<uint32_t>(1_bytes);
    } else if (b && (*b & 0xE0U) == 0xC0U) {
      const uint32_t codePoint = (read<uint32_t>(1_bytes) & 0x1FU) << 6U;
      return codePoint | (read<uint32_t>(1_bytes) & 0x3FU);
    } else if (b && (*b & 0xF0U) == 0xE0U) {
      uint32_t codePoint = (read<uint32_t>(1_bytes) & 0x0FU) << 12U;
      codePoint |= (read<uint32_t>(1_bytes) & 0x3FU) << 6U;
      return codePoint | (read<uint32_t>(1_bytes) & 0x3FU);
    } else if (b && (*b & 0xF8U) == 0xF0U) {
      uint32_t codePoint = (read<uint32_t>(1_bytes) & 0x07U) << 18U;
      codePoint |= (read<uint32_t>(1_bytes) & 0x3FU) << 12U;
      codePoint |= (read<uint32_t>(1_bytes) & 0x3FU) << 6U;
      return codePoint | (read<uint32_t>(1_bytes) & 0x3FU);
    } else if (b && (*b & 0xC0U) == 0x80U) [[unlikely]] {
      // start within the previous UTF-8 character, cannot decode, so skip remainder
      while ((b = peek(1_bytes)) && (*b & 0xC0U) == 0x80U) {
        readByte();
      }
      return 0;
    }
    throw EndOfStreamError("Cannot read more bytes, end of input reached");
  }

  char32_t BitReader::readUtf16CodePoint() {
    auto b = peek(2_bytes);
    if (b && (*b < 0xD800U || *b >= 0xE000U)) {
      return read<uint32_t>(2_bytes);
    } else if (b && (*b & 0xDC00U) == 0xD800U) {
      const uint32_t codePoint = (read<uint32_t>(2_bytes) & 0x3FFU) << 10U;
      return 0x10000U | codePoint | (read<uint32_t>(2_bytes) & 0x3FFU);
    } else if (b && (*b & 0xDC00U) == 0xDC00U) [[unlikely]] {
      // start within the previous UTF-16 character, cannot decode, so skip remainder
      while ((b = peek(2_bytes)) && (*b & 0xDC00U) == 0xDC00U) {
        read(2_bytes);
      }
      return 0;
    }
    throw EndOfStreamError("Cannot read more bytes, end of input reached");
  }

  uint32_t BitReader::readFibonacci() {
    auto [value, numBits] = assertImpl(impl).readUntilTwoOnes();
    return decodeFibonacci(invertBits(value, numBits));
  }

  int32_t BitReader::readSignedFibonacci() {
    auto [value, numBits] = assertImpl(impl).readUntilTwoOnes();
    return decodeNegaFibonacci(invertBits(value, numBits));
  }

  void BitReader::skip(BitCount numBits) {
    if (!numBits && impl) [[unlikely]] {
      return;
    }
    assertImpl(impl).skip(numBits);
  }

  ByteCount BitReader::skipBytesUntil(std::span<const std::byte> pattern) {
    assertAlignment(BYTE_SIZE);
    return assertImpl(impl).readBytesUntil(pattern, nullptr);
  }

} // namespace bml
