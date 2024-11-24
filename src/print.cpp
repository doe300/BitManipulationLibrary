#include "print.hpp"

#include <iomanip>

namespace bml::detail {
  std::ostream &operator<<(std::ostream &os, const PrintView<std::byte> &view) {
    return os << "0x" << std::setfill('0') << std::hex << std::setw(2) << static_cast<unsigned>(view.value) << std::dec;
  }

  std::ostream &operator<<(std::ostream &os, const PrintView<std::span<const std::byte>> &view) {
    os << view.value.size() << " [";
    for (auto entry : view.value)
      os << "0x" << std::setfill('0') << std::hex << std::setw(2) << static_cast<unsigned>(entry) << ", ";
    return os << std::dec << ']';
  }

} // namespace bml::detail
