include(../cmake/cpptest_lite.cmake)

add_executable(TestBml bml.cpp)
target_link_libraries(TestBml PRIVATE bml cpptest-lite)
target_include_directories(TestBml PRIVATE ../src)
add_test(NAME TestBml COMMAND TestBml)
