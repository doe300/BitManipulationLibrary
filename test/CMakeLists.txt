include(../cmake/cpptest_lite.cmake)

add_executable(TestBml bml.cpp binary_map.cpp types.cpp)
target_link_libraries(TestBml PRIVATE bml cpptest-lite)
target_include_directories(TestBml PRIVATE ../src)
target_compile_options(TestBml PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
add_test(NAME TestBml COMMAND TestBml)

if(BML_BUILD_EXAMPLES)
  add_test(NAME StringMaps COMMAND StringMaps)
endif()

if(BML_BUILD_EBML)
  add_subdirectory(ebml)
endif()
