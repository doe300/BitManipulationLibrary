#pragma once

#include "sizes.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>

namespace bml {

  /**
   * Main reader class wrapping a source of bytes (e.g. a buffer or stream) and providing functions to extract bit-,
   * byte-sized and encoded numerical values.
   *
   * NOTE: All values are read in big endian, MSB first! This also means that bits are read from the highest bit first!
   *
   * NOTE: Any read function will throw an error if not enough bytes are available in the underlying byte source.
   */
  class BitReader {
  public:
    class ReaderImpl;

    /**
     * A byte source providing a single byte at a time in its output parameter.
     *
     * Returns whether a next valid byte was returned (true) or no more binary data is available (false).
     */
    using ByteGenerator = std::function<bool(std::byte &)>;

    /**
     * A byte source wrapping a read-only contiguous range of bytes in memory.
     *
     * NOTE: The underlying memory range must be kept alive until the BitReader is done reading, the memory is not
     * copied!
     */
    using ByteRange = std::span<const std::byte>;

    BitReader() noexcept;
    explicit BitReader(ByteGenerator &&generator);
    explicit BitReader(ByteRange range);
    explicit BitReader(const uint8_t *begin, const uint8_t *end) : BitReader(std::as_bytes(std::span{begin, end})) {}
    explicit BitReader(const std::byte *begin, const std::byte *end) : BitReader(ByteRange{begin, end}) {}
    explicit BitReader(std::span<const uint8_t> range) : BitReader(std::as_bytes(range)) {}

    // Disallow copying, since the different byte sources might behave differently when being copied
    BitReader(const BitReader &) = delete;
    BitReader(BitReader &&) noexcept = default;
    ~BitReader() noexcept;

    BitReader &operator=(const BitReader &) = delete;
    BitReader &operator=(BitReader && other) noexcept;

    /**
     * Returns the number of bits already read.
     *
     * If the start of the bit source is properly aligned, this can also be used to determine the current alignment.
     */
    BitCount position() const noexcept;

    /*
     * Returns whether there is at least one more byte to read in the underlying byte source.
     */
    bool hasMoreBytes() noexcept;

    /**
     * Reads and drops all bits until the given bit alignment is achieved.
     *
     * Returns the number of bits skipped.
     */
    BitCount skipToAligment(BitCount bitAlignment);

    /**
     * Throws an exception if the current read position is not aligned to the given bit alignment.
     */
    void assertAlignment(BitCount bitAlignment);

    /**
     * Reads a single bit.
     */
    bool read();

    /**
     * Peeks the given amount of bits without increasing the current read position.
     */
    std::uintmax_t peek(BitCount numBits);

    /**
     * Reads the given amount of bits, increasing the current read position.
     */
    std::uintmax_t read(BitCount numBits);

    template <typename T>
    T read(BitCount numBits)
      requires(std::is_unsigned_v<T> || std::is_enum_v<T>)
    {
      return static_cast<T>(read(numBits));
    }

    /**
     * Reads the given amount of aligned bytes.
     *
     * Throws an exception if the read position is not byte aligned, see assertAlignment().
     */
    std::uintmax_t readBytes(ByteCount numBytes);

    /**
     * Reads a single aligned byte.
     *
     * Throws an exception if the read position is not byte aligned, see assertAlignment().
     */
    std::byte readByte() { return static_cast<std::byte>(readBytes(1_bytes)); }

    template <typename T>
    T readBytes(ByteCount numBytes)
      requires(std::is_unsigned_v<T> || std::is_enum_v<T>)
    {
      return static_cast<T>(readBytes(numBytes));
    }

    /**
     * Reads as many bytes required to fill the given output range completely.
     *
     * Throws an exception if the read position is not byte aligned, see assertAlignment().
     */
    void readBytesInto(std::span<std::byte> outBytes);

    /**
     * Reads an unsigned Exponential-Golomb encoded value and returns the underlying decoded value.
     */
    std::uintmax_t readExpGolomb();

    template <typename T>
    T readExpGolomb()
      requires(std::is_unsigned_v<T>)
    {
      return static_cast<T>(readExpGolomb());
    }

    /**
     * Reads an signed Exponential-Golomb encoded value and returns the underlying decoded value.
     */
    std::intmax_t readSignedExpGolomb();

    template <typename T>
    T readSignedExpGolomb()
      requires(std::is_signed_v<T>)
    {
      return static_cast<T>(readSignedExpGolomb());
    }

    /**
     * Reads as many bytes required to produce a full UTF-8 encoded Unicode code point.
     *
     * Throws an exception if the read position is not byte aligned, see assertAlignment().
     */
    char32_t readUtf8CodePoint();

    /**
     * Reads as many bytes required to produce a full UTF-16 encoded Unicode code point.
     *
     * Throws an exception if the read position is not byte aligned, see assertAlignment().
     */
    char32_t readUtf16CodePoint();

    /**
     * Reads an unsigned Fibonacci encoded value and returns the underlying decoded value.
     */
    uint32_t readFibonacci();

    template <typename T>
    T readFibonacci()
      requires(std::is_unsigned_v<T>)
    {
      return static_cast<T>(readFibonacci());
    }

    /**
     * Reads an signed Negafibonacci encoded value and returns the underlying decoded value.
     */
    int32_t readSignedFibonacci();

    template <typename T>
    T readSignedFibonacci()
      requires(std::is_signed_v<T>)
    {
      return static_cast<T>(readSignedFibonacci());
    }

    /**
     * Skips the given number of bits for reading, incrementing the current read position by the given number of bits.
     */
    void skip(BitCount numBits);

  private:
    std::unique_ptr<ReaderImpl> impl;
  };

} // namespace bml
