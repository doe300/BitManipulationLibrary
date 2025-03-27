#include "data_range.hpp"

#include "errors.hpp"

#include <compare>
#include <type_traits>

namespace bml {
  static_assert(sizeof(DataRange) <= 4 * sizeof(void *));

  template <typename T>
  static auto compare(const T &one, const T &other) noexcept {
    return one <=> other;
  }

  static auto compare(const std::span<const std::byte> &one, const std::span<const std::byte> &other) noexcept {
    if (one.size() < other.size()) {
      return std::strong_ordering::less;
    } else if (one.size() > other.size()) {
      return std::strong_ordering::greater;
    }
    return one.data() <=> other.data();
  }

  bool DataRange::operator==(const DataRange &other) const noexcept {
    return std::visit(
        [](const auto &first, const auto &second) {
          using T = std::decay_t<decltype(first)>;
          using U = std::decay_t<decltype(second)>;
          if constexpr (std::is_same_v<T, U>) {
            return compare(first, second) == std::partial_ordering::equivalent;
          } else {
            return false;
          }
        },
        content, other.content);
  }

  bool DataRange::operator<(const DataRange &other) const noexcept {
    if (content.index() < other.content.index()) {
      return true;
    } else if (content.index() > other.content.index()) {
      return false;
    }
    return std::visit(
        [](const auto &first, const auto &second) {
          using T = std::decay_t<decltype(first)>;
          using U = std::decay_t<decltype(second)>;
          if constexpr (std::is_same_v<T, U>) {
            return compare(first, second) == std::partial_ordering::less;
          } else {
            return false;
          }
        },
        content, other.content);
  }

  bool DataRange::empty() const noexcept {
    return std::visit([](const auto &val) { return val.empty(); }, content);
  }

  std::size_t DataRange::size() const noexcept {
    return std::visit(
        [](const auto &val) {
          using T = std::decay_t<decltype(val)>;
          if constexpr (std::is_same_v<T, KnownRange>) {
            return val.size.num;
          } else {
            return val.size();
          }
        },
        content);
  }

  ByteCount DataRange::numBytes() const noexcept {
    return std::visit(
        [](const auto &val) {
          using T = std::decay_t<decltype(val)>;
          if constexpr (std::is_same_v<T, KnownRange>) {
            return val.size;
          } else {
            return ByteCount{val.size()};
          }
        },
        content);
  }

  bool DataRange::hasData() const noexcept {
    return std::visit(
        [](const auto &val) {
          using T = std::decay_t<decltype(val)>;
          if constexpr (std::is_same_v<T, KnownRange>) {
            return false;
          } else {
            return true;
          }
        },
        content);
  }

  ByteRange DataRange::byteRange() const noexcept {
    return std::visit(
        [](const auto &val) {
          using T = std::decay_t<decltype(val)>;
          if constexpr (std::is_same_v<T, KnownRange>) {
            return val;
          } else {
            return ByteRange{};
          }
        },
        content);
  }

  std::span<const std::byte> DataRange::data() const noexcept {
    return std::visit(
        [](const auto &val) {
          using T = std::decay_t<decltype(val)>;
          if constexpr (std::is_same_v<T, KnownRange>) {
            return std::span<const std::byte>{};
          } else {
            return std::span{val};
          }
        },
        content);
  }

  DataRange::Mode DataRange::mode() const noexcept {
    return std::visit(
        [](const auto &val) {
          using T = std::decay_t<decltype(val)>;
          if constexpr (std::is_same_v<T, KnownRange>) {
            return Mode::KNOWN;
          } else if constexpr (std::is_same_v<T, BorrowedRange>) {
            return Mode::BORROWED;
          } else if constexpr (std::is_same_v<T, OwnedRange>) {
            return Mode::OWNED;
          }
        },
        content);
  }

  DataRange DataRange::borrow() const noexcept {
    return std::visit(
        [](const auto &val) {
          using T = std::decay_t<decltype(val)>;
          if constexpr (std::is_same_v<T, OwnedRange>) {
            return DataRange{std::span{val}};
          } else {
            return DataRange{val};
          }
        },
        content);
  }

  std::ostream &operator<<(std::ostream &os, const DataRange &range) {
    std::visit([&os](const auto &val) { os << printView(val); }, range.content);
    return os;
  }

  void fillDataRange(DataRange &range, std::span<const std::byte> data, DataRange::Mode targetMode) {
    if (range.mode() >= targetMode) {
      return;
    }

    auto tmp = range.data();
    if (auto byteRange = range.byteRange()) {
      tmp = byteRange.applyTo(data);
      if (tmp.size() < byteRange.size.num) {
        throw EndOfStreamError("Data range " + byteRange.toString() + " lies outside of " +
                               std::to_string(data.size()) + " bytes of data");
      }
    }

    if (targetMode == DataRange::Mode::OWNED) {
      range = DataRange(std::vector<std::byte>(tmp.begin(), tmp.end()));
    } else if (targetMode == DataRange::Mode::BORROWED) {
      range = DataRange{tmp};
    }
  }

  void fillDataRange(DataRange &range, std::istream &input) {
    switch (range.mode()) {
    case DataRange::Mode::KNOWN: {
      auto knownRange = range.byteRange();
      std::vector<std::byte> buffer(knownRange.size.num);
      input.seekg(static_cast<std::ios::off_type>(knownRange.offset.num), std::ios::beg);
      input.read(reinterpret_cast<char *>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
      if (input.eof()) {
        throw EndOfStreamError("Error reading data range " + knownRange.toString() + " from input stream");
      }
      range = DataRange{std::move(buffer)};
      break;
    }
    case DataRange::Mode::BORROWED: {
      auto tmp = range.data();
      range = DataRange(std::vector<std::byte>(tmp.begin(), tmp.end()));
      break;
    }
    case DataRange::Mode::OWNED:
      break;
    }
  }
} // namespace bml
