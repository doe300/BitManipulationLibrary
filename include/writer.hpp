#pragma once

#include "sizes.hpp"

#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <span>
#include <variant>

namespace bml {
  /**
   * Main writer class wrapping a sink of bytes (e.g. a buffer or stream) and providing functions to write bit-,
   * byte-sized and encoded numerical values.
   *
   * NOTE: All values are written in big endian, MSB first! This also means that bits are written from the highest bit
   * first!
   *
   * NOTE: Any write function will throw an error if the underlying byte sink has not enough space left.
   */
  class BitWriter {
  public:
    /**
     * A byte sink consuming a single byte at a time.
     *
     * Returns whether the byte was consumed (true) or no more binary data can be accepted (false).
     */
    using ByteConsumer = std::function<bool(std::byte)>;

    /**
     * A byte sink wrapping a writeable contiguous range of bytes in memory.
     *
     * NOTE: The underlying memory range must be kept alive until the BitWriter is done writing!
     */
    struct ByteRange {
      std::byte *begin;
      std::byte *end;
    };

    using ByteSink = std::variant<std::monostate, ByteConsumer, ByteRange>;

    static constexpr ByteCount BYTE_SIZE{1};
    static constexpr BitCount CACHE_SIZE{static_cast<std::size_t>(std::numeric_limits<std::uintmax_t>::digits)};

    BitWriter() noexcept = default;
    BitWriter(ByteConsumer &&consumer) noexcept : sink(std::move(consumer)) {}
    BitWriter(uint8_t *begin, uint8_t *end) noexcept
        : sink(ByteRange{reinterpret_cast<std::byte *>(begin), reinterpret_cast<std::byte *>(end)}) {}
    BitWriter(std::byte *begin, std::byte *end) noexcept : sink(ByteRange{begin, end}) {}

    BitWriter(std::span<uint8_t> range) noexcept
        : sink(ByteRange{reinterpret_cast<std::byte *>(range.data()),
                         reinterpret_cast<std::byte *>(range.data() + range.size())}) {}
    BitWriter(std::span<std::byte> range) noexcept : sink(ByteRange{range.data(), range.data() + range.size()}) {}

    // Disallow copying, since the different byte sinks might behave differently when being copied
    BitWriter(const BitWriter &) = delete;
    BitWriter(BitWriter &&) noexcept = default;
    ~BitWriter() noexcept = default;

    BitWriter &operator=(const BitWriter &) = delete;
    BitWriter &operator=(BitWriter &&) noexcept = default;

    /**
     * Returns the number of bits already written.
     *
     * If the start of the bit sink is properly aligned, this can also be used to determine the current alignment.
     */
    BitCount position() const noexcept { return bytesWritten + cacheSize; }

    /**
     * Writes the given bit value until the given bit alignment is achieved.
     *
     * Returns the number of bits written.
     */
    BitCount fillToAligment(BitCount bitAlignment, bool bit);

    /**
     * Throws an exception if the current write position is not aligned to the given bit alignment.
     */
    void assertAlignment(BitCount bitAlignment);

    /**
     * Writes a single bit.
     */
    void write(bool bit) { write(bit ? 1u : 0u, 1_bits); }

    /**
     * Writes the given number of bits of the given value.
     *
     * NOTE: Given a number of bits of N, the lower N bits written!
     */
    void write(std::uintmax_t value, BitCount numBits);

    /**
     * Writes the given amount of aligned bytes.
     *
     * Throws an exception if the write position is not byte aligned, see assertAlignment().
     */
    void writeBytes(std::uintmax_t value, ByteCount numBytes);

    /**
     * Writes a single aligned byte.
     *
     * Throws an exception if the write position is not byte aligned, see assertAlignment().
     */
    void writeByte(std::byte byte) { writeBytes(static_cast<uint8_t>(byte), BYTE_SIZE); }

    /**
     * Writes all bytes in the given input range.
     *
     * Throws an exception if the write position is not byte aligned, see assertAlignment().
     */
    void writeBytes(std::span<const std::byte> bytes);

    /**
     * Encodes the given value with the unsigned Exponential-Golomb coding and writes it to the byte sink.
     */
    void writeExpGolomb(std::uintmax_t value);

    /**
     * Encodes the given value with the signed Exponential-Golomb coding and writes it to the byte sink.
     */
    void writeSignedExpGolomb(std::intmax_t value);

    /**
     * Writes as many bytes required to UTF-8 encode the given Unicode code point.
     *
     * Throws an exception if the write position is not byte aligned, see assertAlignment().
     */
    void writeUtf8CodePoint(char32_t codePoint);

    /**
     * Writes as many bytes required to UTF-16 encode the given Unicode code point.
     *
     * Throws an exception if the write position is not byte aligned, see assertAlignment().
     */
    void writeUtf16CodePoint(char32_t codePoint);

    /**
     * Encodes the given value with the Fibonacco coding and writes it to the byte sink.
     */
    void writeFibonacci(uint32_t value);

    /**
     * Encodes the given value with the Negafibonacco coding and writes it to the byte sink.
     */
    void writeSignedFibonacci(int32_t value);

  private:
    void flushFullBytes();

  private:
    ByteCount bytesWritten{};
    BitCount cacheSize{};
    /** is always left-adjusted to write the highest bits first */
    std::uintmax_t cache = 0;
    ByteSink sink;
  };

} // namespace bml
