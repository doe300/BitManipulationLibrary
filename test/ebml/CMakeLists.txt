add_executable(TestEbml ebml.cpp mkv.cpp)
target_link_libraries(TestEbml PRIVATE bml ebml cpptest-lite)
add_test(NAME TestEbml COMMAND TestEbml)
