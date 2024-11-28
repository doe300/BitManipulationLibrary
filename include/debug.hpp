#pragma once

#include <iostream>
#include <source_location>
#include <string>

namespace bml {

  class Debug {
  public:
    static void error(const std::string &message, std::source_location location = std::source_location::current()) {
      std::cerr << "Error in " << location.file_name() << ':' << location.line() << ": " << message << std::endl;
    }
  };

} // namespace bml
