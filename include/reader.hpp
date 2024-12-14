#pragma once

#include "sizes.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <span>
#include <variant>

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
    struct ByteRange {
      const std::byte *begin;
      const std::byte *end;
    };

    using ByteSource = std::variant<std::monostate, ByteGenerator, ByteRange>;

    static constexpr ByteCount BYTE_SIZE{1};
    static constexpr BitCount CACHE_SIZE{static_cast<std::size_t>(std::numeric_limits<std::uintmax_t>::digits)};

    BitReader() noexcept = default;
    BitReader(ByteGenerator &&generator) noexcept : source(std::move(generator)) {}
    BitReader(const uint8_t *begin, const uint8_t *end) noexcept
        : source(ByteRange{reinterpret_cast<const std::byte *>(begin), reinterpret_cast<const std::byte *>(end)}) {}
    BitReader(const std::byte *begin, const std::byte *end) noexcept : source(ByteRange{begin, end}) {}

    BitReader(std::span<const uint8_t> range) noexcept
        : source(ByteRange{reinterpret_cast<const std::byte *>(range.data()),
                           reinterpret_cast<const std::byte *>(range.data() + range.size())}) {}
    BitReader(std::span<const std::byte> range) noexcept
        : source(ByteRange{range.data(), range.data() + range.size()}) {}

    // Disallow copying, since the different byte sources might behave differently when being copied
    BitReader(const BitReader &) = delete;
    BitReader(BitReader &&) noexcept = default;
    ~BitReader() noexcept = default;

    BitReader &operator=(const BitReader &) = delete;
    BitReader &operator=(BitReader &&) noexcept = default;

    /**
     * Returns the number of bits already read.
     *
     * If the start of the bit source is properly aligned, this can also be used to determine the current alignment.
     */
    BitCount position() const noexcept { return bytesRead - cacheSize; }

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
    void makeAvailable(BitCount numBits, bool throwOnEos = true);

    /**
     * Reads until the uppermost bit in the cache is set and return the number of leading zero bits.
     */
    BitCount readLeadingZeroes();
    /**
     * Reads until two consecutive one bits are set and return the value with then number of bits read.
     */
    EncodedValue<uint64_t> readUntilTwoOnes();

  private:
    ByteCount bytesRead{};
    BitCount cacheSize{};
    /** is always left-adjusted to read the highest bits first */
    std::uintmax_t cache = 0;
    ByteSource source;
  };

} // namespace bml
