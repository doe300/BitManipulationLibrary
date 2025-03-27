#pragma once

#include "ebml.hpp"
#include "mkv.hpp"

#include <coroutine>
#include <optional>

namespace bml::ebml {
  class ChunkedReader::promise_type {
  public:
    ChunkedReader get_return_object() { return {std::coroutine_handle<promise_type>::from_promise(*this)}; }

    // NOTE: Needs to be suspend_never, otherwise references passed to coroutine would dangle immediately (since the
    // routine is suspensed before the function body is invoked)
    std::suspend_never initial_suspend() noexcept { return {}; }
    // NOTE: suspend_always makes the calling code responsible for cleaning up the coroutine state (i.e. the
    // ChunkedReader destructor), but also allows calling code to still access the stat (e.g. the last result).
    // suspend_never would directly clean up the coroutine state instead.
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() { error = std::current_exception(); }

    void return_void() noexcept { lastId.reset(); }

    std::suspend_always yield_value(std::optional<ElementId> val) noexcept {
      lastId = val;
      return {};
    }

    std::optional<ElementId> lastId;
    std::exception_ptr error;
  };

  template <typename Func = void (*)(BitWriter &)>
  void writeElement(BitWriter &writer, ElementId id, Func &&writeContent,
                    std::size_t estimatedSize = sizeof(uintmax_t)) {
    std::vector<std::byte> buffer{};
    buffer.reserve(estimatedSize);
    BitWriter bufWriter{buffer, BitWriter::GROW};
    writeContent(bufWriter);
    bufWriter.flush();

    detail::writeElementHeader(writer, id, ByteCount{buffer.size()});
    writer.writeBytes(buffer);
  }

  namespace detail {
    BitReader &getUnderlyingReader(BitReader &reader);
  } // namespace detail

  namespace mkv {
    std::vector<DataRange> readFrameRanges(BitReader &reader, const BlockHeader &header, const ByteRange &dataRange,
                                           bool copyFrameData);
    void writeFrameRanges(BitWriter &writer, const std::vector<DataRange> &frameRanges, BlockHeader::Lacing lacing);
  } // namespace mkv
} // namespace bml::ebml
