#pragma once

#include "helper.hpp"
#include "sizes.hpp"

#include <iostream>
#include <ranges>
#include <span>
#include <variant>
#include <vector>

namespace bml {
  /**
   * Representation of a contiguous range of raw binary data.
   *
   * For performance and memory consumption reasons, data ranges can reference the underlying data in different ways
   * from simply keeping track of where in the original data source (e.g. a file) the underlying data is located to
   * owning their own copy of the in-memory data.
   */
  class DataRange {
  public:
    enum class Mode : uint8_t {
      /**
       * The position and size in the original source is known, no raw bytes are referenced.
       */
      KNOWN,
      /**
       * The corresponding raw bytes are referenced in the underlying in-memory data store.
       */
      BORROWED,
      /**
       * The data range owns its own copy of its contained raw bytes.
       */
      OWNED,
    };

    constexpr DataRange() noexcept = default;

    /**
     * Wraps a known byte range.
     */
    DataRange(const ByteRange &knownRange) noexcept : content{knownRange} {}

    /**
     * Wraps a borrowed in-memory range of raw bytes.
     *
     * NOTE: Since the given range is borrowed, care needs to be taken to keep the underlying memory alive as long as
     * this data range is accessed.
     */
    DataRange(std::span<const std::byte> borrowedRange) noexcept : content{borrowedRange} {}

    /**
     * Wraps an owned in-memory range of raw bytes.
     */
    DataRange(std::vector<std::byte> &&ownedRange) noexcept : content{std::move(ownedRange)} {}

    /** Delete to force disambiguation between borrowing and owning range constructors */
    DataRange(const std::vector<std::byte> &) = delete;

    bool operator==(const DataRange &other) const noexcept;
    bool operator<(const DataRange &other) const noexcept;

    /**
     * Returns whether this object represents a non-empty range, i.e. refers to a non-zero number of bytes.
     */
    explicit operator bool() const noexcept { return !empty(); }

    /**
     * Returns whether this object represents an empty range, e.g. one without any bytes in it.
     */
    bool empty() const noexcept;

    /**
     * Returns the number of bytes of the represented range.
     */
    std::size_t size() const noexcept;
    ByteCount numBytes() const noexcept;

    /**
     * Returns whether this object directly references the underlying bytes, e.g. as a borrowed or owned byte range.
     *
     * A data range might refer to a specific range of data without having a reference to the data itself, in which case
     * the data range will not be empty(), but will return false here.
     */
    bool hasData() const noexcept;

    /**
     * Returns the known data range or an empty range if this object does not store a known data range (e.g. BORROWED or
     * OWNED modes).
     */
    ByteRange byteRange() const noexcept;

    /**
     * Returns a read-only view to the directly referenced data or an empty span if no data is directly referenced (e.g.
     * KNOWN mode).
     */
    std::span<const std::byte> data() const noexcept;

    /**
     * Returns the current mode of this data range object
     */
    Mode mode() const noexcept;

    /**
     * Returns a copy of this data range representing the same underlying bytes.
     *
     * If this data range owns the underlying memory, a BORROWED range is returned referencing this range's data
     * storage. If this data range does not own the referenced memory, a simple copy of this object is returned.
     *
     * NOTE: Since the returned range is borrowed, care needs to be taken to keep the underlying memory alive as long as
     * this data range is accessed.
     */
    DataRange borrow() const noexcept;

    friend std::ostream &operator<<(std::ostream &os, const DataRange &range);

  private:
    using KnownRange = ByteRange;
    using BorrowedRange = std::span<const std::byte>;
    using OwnedRange = std::vector<std::byte>;
    std::variant<KnownRange, BorrowedRange, OwnedRange> content;
  };

  /**
   * Fills the given input data range with the corresponding bytes extracted from the given input bytes.
   *
   * Depending on the given target mode, the given data range is modified to borrow or own its referenced raw bytes as
   * read from the given in-memory data. If the given data range's mode is stronger than the given target mode, the data
   * range is not modified.
   *
   * NOTE: The given input bytes MUST MATCH the original raw bytes the given data range object was created over.
   *
   * NOTE: If the target mode is BORROWED, the given input span needs to outlive any access to the updated data range.
   *
   * NOTE: Throws an EndOfStreamError exception if the data range cannot be filled by the given input data, i.e. due to
   * the referenced byte range being outside of the input data.
   */
  void fillDataRange(DataRange &range, std::span<const std::byte> data,
                     DataRange::Mode targetMode = DataRange::Mode::OWNED);

  /**
   * Fills the given input data range with the corresponding bytes extracted from the given input stream.
   *
   * This function will convert all non-empty data ranges into OWNED data ranges.
   *
   * NOTE: The given input bytes MUST MATCH the original raw bytes the given data range object was created over.
   *
   * NOTE: Throws an EndOfStreamError exception if the data range cannot be filled by the given input stream, i.e. due
   * to the referenced byte range being outside of the input data.
   */
  void fillDataRange(DataRange &range, std::istream &input);

  /**
   * View for amending a DataRange parent object read from the underlying input range with the DataRange's data
   * extracted from the associated data source.
   *
   * NOTE: The produced DataRange's data might directly reference the underlying data source (i.e. for in-memory data
   * buffers) and thus MUST NOT outlive the lifetime of the underlying data source.
   */
  template <std::ranges::input_range V, typename Source>
  class FillDataRangeView : public std::ranges::view_interface<FillDataRangeView<V, Source>> {
    using InnerIterator = std::ranges::iterator_t<V>;
    using MemberAccess = DataRange std::ranges::range_value_t<V>::*;

    struct Sentinel {
      std::ranges::sentinel_t<V> inner;
    };

    class Iterator {
    public:
      using value_type = typename std::ranges::range_value_t<V>;
      using difference_type = std::ptrdiff_t;

      explicit Iterator() noexcept = default;
      Iterator(InnerIterator begin, Source source, MemberAccess member) noexcept
          : iterator(begin), dataSource(source), dataMember(member) {}

      Iterator &operator++() {
        ++iterator;
        return *this;
      }

      Iterator operator++(int) {
        auto copy = *this;
        ++iterator;
        return copy;
      }

      value_type operator*() const {
        auto object = *iterator;
        fillDataRange(object.*dataMember, dataSource);
        return object;
      }

      bool operator==(const Iterator &other) const noexcept { return iterator == other.iterator; }

      friend bool operator==(const Sentinel &sentinel, const Iterator &it) noexcept {
        return it.iterator == sentinel.inner;
      }

    private:
      InnerIterator iterator;
      Source dataSource;
      MemberAccess dataMember;
    };

  public:
    FillDataRangeView(V &&input, Source source, MemberAccess member) noexcept
        : inputRange(std::forward<V>(input)), dataSource(source), dataMember(member) {}

    Iterator begin() const { return Iterator{inputRange.begin(), dataSource, dataMember}; }
    Sentinel end() const { return Sentinel{inputRange.end()}; }

  private:
    V inputRange;
    Source dataSource;
    MemberAccess dataMember;
  };

  /**
   * Range adaptor to produce a FillDataRangeView amending an input range using the additional data source.
   */
  template <typename T, typename Source>
  struct FillDataRange {
    using MemberAccess = DataRange T::*;

    template <std::ranges::input_range V>
    friend FillDataRangeView<V, Source> operator|(V &&range, const FillDataRange &adaptor) noexcept {
      return adaptor(std::forward<V>(range));
    }

    template <std::ranges::input_range V>
    FillDataRangeView<V, Source> operator()(V &&range) const noexcept {
      return FillDataRangeView<V, Source>{std::forward<V>(range), dataSource, dataMember};
    }

    /**
     * Directly amend the given input DataRange parent object with the DataRange's data as provided by the associated
     * data source.
     *
     * NOTE: The produced data might directly reference the underlying data source (i.e. for in-memory data buffers) and
     * thus MUST NOT outlive the lifetime of the underlying data source.
     */
    T operator()(T object) {
      fillDataRange(object.*dataMember, dataSource);
      return object;
    }

    Source dataSource;
    MemberAccess dataMember;
  };
} // namespace bml
