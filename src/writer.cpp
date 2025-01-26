
#include "writer.hpp"

#include "common.hpp"
#include "errors.hpp"
#include "helper.hpp"

#include <algorithm>
#include <stdexcept>

namespace bml {
  class BitWriter::WriterImpl {
  public:
    WriterImpl() = default;
    WriterImpl(const WriterImpl &) = delete;
    WriterImpl(WriterImpl &&) noexcept = delete;
    virtual ~WriterImpl() noexcept = default;

    WriterImpl &operator=(const WriterImpl &) = delete;
    WriterImpl &operator=(WriterImpl &&) noexcept = delete;

    BitCount position() const noexcept { return sinkBytesWritten + cacheSize; }

    void write(std::uintmax_t value, BitCount numBits) {
      if ((numBits + cacheSize) > CACHE_SIZE) {
        // split up the bits to write into multiple calls
        auto upper = value >> (CACHE_SIZE / 2u);
        write(upper, numBits - CACHE_SIZE / 2u);
        auto lower = value & ((std::uintmax_t{1} << (CACHE_SIZE / 2u)) - 1u);
        write(lower, CACHE_SIZE / 2u);
        return;
      }
      cacheSize += numBits;
      cache |= value << (CACHE_SIZE - cacheSize);
      flushFullBytes();
    }

    void writeBytes(std::span<const std::byte> bytes) {
      flushFullBytes();
      if (cacheSize) {
        throw std::invalid_argument("Output bit stream is not properly aligned");
      }
      if (!sinkBytes(bytes)) {
        throw EndOfStreamError("Cannot write more bytes, end of output reached");
      }
      sinkBytesWritten += ByteCount{bytes.size()};
    }

    void fillBytes(std::byte value, ByteCount numBytes) {
      flushFullBytes();
      if (cacheSize) {
        throw std::invalid_argument("Output bit stream is not properly aligned");
      }
      if (!sinkCopies(value, numBytes)) {
        throw EndOfStreamError("Cannot write more bytes, end of output reached");
      }
      sinkBytesWritten += numBytes;
    }

    void flushFullBytes() {
      Cache tmp{cache, cacheSize};
      sinkBytesWritten += flushFullCacheBytes(
          tmp, [this](std::byte nextByte) { return sinkByte(nextByte); },
          []() { throw EndOfStreamError("Cannot write more bytes, end of output reached"); });
      cache = tmp.value;
      cacheSize = tmp.size;
    }

  protected:
    [[nodiscard]] virtual bool sinkByte(std::byte byte) { return sinkBytes(std::span{&byte, 1}); }
    [[nodiscard]] virtual bool sinkBytes(std::span<const std::byte> bytes) = 0;

    [[nodiscard]] virtual bool sinkCopies(std::byte value, ByteCount numBytes) {
      for (auto i = 0_bytes; i < numBytes; i += 1_bytes) {
        if (!sinkByte(value)) {
          return false;
        }
      }
      return true;
    }

  private:
    ByteCount sinkBytesWritten;
    BitCount cacheSize;
    /** is always left-adjusted to write the highest bits first */
    std::uintmax_t cache = 0;
  };

  struct ByteConsumerImpl final : BitWriter::WriterImpl {
    explicit ByteConsumerImpl(BitWriter::ByteConsumer &&consumer) : consumeByte(std::move(consumer)) {}

    bool sinkByte(std::byte byte) override { return consumeByte(byte); }

    bool sinkBytes(std::span<const std::byte> bytes) override {
      return std::all_of(bytes.begin(), bytes.end(), consumeByte);
    }

    BitWriter::ByteConsumer consumeByte;
  };

  struct ByteWriteRangeImpl final : BitWriter::WriterImpl {
    explicit ByteWriteRangeImpl(BitWriter::ByteRange range) : sinkRange(range) {}

    bool sinkBytes(std::span<const std::byte> bytes) override {
      if (bytes.size() > sinkRange.size()) {
        return false;
      }
      std::copy_n(bytes.begin(), sinkRange.size(), sinkRange.begin());
      sinkRange = sinkRange.subspan(bytes.size());
      return true;
    }

    bool sinkCopies(std::byte value, ByteCount numBytes) override {
      if (numBytes.num > sinkRange.size()) {
        return false;
      }
      std::fill_n(sinkRange.begin(), numBytes.num, value);
      sinkRange = sinkRange.subspan(numBytes.num);
      return true;
    }

    BitWriter::ByteRange sinkRange;
  };

  struct ByteOutputStreamImpl final : BitWriter::WriterImpl {
    explicit ByteOutputStreamImpl(std::ostream &os) : output(os) { output.exceptions(std::ios::badbit); }

    bool sinkByte(std::byte byte) override {
      output.write(reinterpret_cast<const char *>(&byte), 1);
      return static_cast<bool>(output);
    }
    bool sinkBytes(std::span<const std::byte> bytes) override {
      output.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
      return static_cast<bool>(output);
    }

    std::ostream &output;
  };

  BitWriter::BitWriter() noexcept = default;
  BitWriter::BitWriter(ByteConsumer &&consumer) : impl(std::make_unique<ByteConsumerImpl>(std::move(consumer))) {}
  BitWriter::BitWriter(ByteRange range) : impl(std::make_unique<ByteWriteRangeImpl>(range)) {}
  BitWriter::BitWriter(std::ostream &os) : impl(std::make_unique<ByteOutputStreamImpl>(os)) {}
  BitWriter::BitWriter(BitWriter &&other) noexcept : impl(std::move(other.impl)) { other.impl.reset(); }
  BitWriter::~BitWriter() noexcept = default;

  BitWriter &BitWriter::operator=(BitWriter &&other) noexcept {
    std::swap(impl, other.impl);
    return *this;
  }

  BitCount BitWriter::position() const noexcept { return impl ? impl->position() : 0_bits; }

  BitCount BitWriter::fillToAligment(BitCount bitAlignment, bool bit) {
    auto numBits = position() % bitAlignment ? (bitAlignment - position() % bitAlignment) : 0_bits;
    auto value = bit ? std::numeric_limits<std::uintmax_t>::max() : 0U;
    write(value, numBits);
    return numBits;
  }

  static BitWriter::WriterImpl &assertImpl(std::unique_ptr<BitWriter::WriterImpl> &ptr) noexcept(false) {
    if (ptr) {
      return *ptr;
    }
    throw std::runtime_error{"Cannot write to empty BitWriter instance"};
  }

  void BitWriter::assertAlignment(BitCount bitAlignment) {
    if (assertImpl(impl).position() % bitAlignment) {
      throw std::invalid_argument("Output bit stream is not properly aligned");
    }
  }

  void BitWriter::write(std::uintmax_t value, BitCount numBits) { assertImpl(impl).write(value, numBits); }

  void BitWriter::writeBytes(std::uintmax_t value, ByteCount numBytes) {
    assertAlignment(BYTE_SIZE);
    write(value, numBytes);
  }

  void BitWriter::writeByte(std::byte byte) { writeBytes(static_cast<uint8_t>(byte), BYTE_SIZE); }

  void BitWriter::writeBytes(std::span<const std::byte> bytes) {
    assertAlignment(BYTE_SIZE);
    assertImpl(impl).writeBytes(bytes);
  }

  void BitWriter::fillBytes(std::byte value, ByteCount numBytes) {
    assertAlignment(BYTE_SIZE);
    assertImpl(impl).fillBytes(value, numBytes);
  }

  void BitWriter::writeExpGolomb(std::uintmax_t value) {
    auto [encodedValue, numBits] = encodeExpGolomb(value);
    write(encodedValue, numBits);
  }

  void BitWriter::writeSignedExpGolomb(std::intmax_t value) {
    auto [encodedValue, numBits] = encodeSignedExpGolomb(value);
    write(encodedValue, numBits);
  }

  void BitWriter::writeUtf8CodePoint(char32_t codePoint) {
    auto utf8String = toUtf8String(codePoint);
    writeBytes(std::as_bytes(std::span{utf8String}));
  }

  void BitWriter::writeUtf16CodePoint(char32_t codePoint) {
    auto value = static_cast<uint32_t>(codePoint);
    if (value < 0x10000U) {
      write(value, 2_bytes);
    } else {
      value -= 0x10000U;
      write(0xD800U + ((value >> 10U) & 0x3FFU), 2_bytes);
      write(0xDC00U + (value & 0x3FFU), 2_bytes);
    }
  }

  void BitWriter::writeFibonacci(uint32_t value) {
    auto [encodedValue, numBits] = encodeFibonacci(value);
    write(invertBits(encodedValue, numBits), numBits);
  }

  void BitWriter::writeSignedFibonacci(int32_t value) {
    auto [encodedValue, numBits] = encodeNegaFibonacci(value);
    write(invertBits(encodedValue, numBits), numBits);
  }

  void BitWriter::flush() { fillToAligment(1_bytes, false); }

} // namespace bml
