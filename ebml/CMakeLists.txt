add_library(ebml ebml.cpp mkv.cpp)
target_include_directories(ebml INTERFACE .)
target_link_libraries(ebml PUBLIC bml)

add_executable(mkv_parser tool.cpp)
target_link_libraries(mkv_parser ebml)
target_precompile_headers(mkv_parser REUSE_FROM ebml)
