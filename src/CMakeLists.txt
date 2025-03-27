
add_library(bml)
target_include_directories(bml PUBLIC ../include)
target_sources(bml PRIVATE
  data_range.cpp
  helper.cpp
  io.cpp
  print.cpp
  reader.cpp
  types.cpp
  writer.cpp
  yaml.cpp
)
target_precompile_headers(bml PUBLIC ../include/bml.hpp)
