#pragma once

#include "sizes.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>

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
    class WriterImpl;

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
    using ByteRange = std::span<std::byte>;

    BitWriter() noexcept;
    explicit BitWriter(ByteConsumer &&consumer);
    explicit BitWriter(ByteRange range);
    explicit BitWriter(uint8_t *begin, uint8_t *end) : BitWriter(std::as_writable_bytes(std::span{begin, end})) {}
    explicit BitWriter(std::byte *begin, std::byte *end) : BitWriter(ByteRange{begin, end}) {}
    explicit BitWriter(std::span<uint8_t> range) : BitWriter(std::as_writable_bytes(range)) {}

    // Disallow copying, since the different byte sinks might behave differently when being copied
    BitWriter(const BitWriter &) = delete;
    BitWriter(BitWriter &&) noexcept = default;
    ~BitWriter() noexcept;

    BitWriter &operator=(const BitWriter &) = delete;
    BitWriter &operator=(BitWriter &&other) noexcept;

    /**
     * Returns the number of bits already written.
     *
     * If the start of the bit sink is properly aligned, this can also be used to determine the current alignment.
     */
    BitCount position() const noexcept;

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
    void writeByte(std::byte byte);

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

    /**
     * Fills the last cached partial byte with trailing zeroes and writes all bytes to the underlying byte sink.
     *
     * This function is equivalent to:
     *   fillToAligment(1_bytes, 0);
     */
    void flush();

  private:
    std::unique_ptr<WriterImpl> impl;
  };

} // namespace bml
