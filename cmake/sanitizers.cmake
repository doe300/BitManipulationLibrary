#
# Set CMAKE_BUILD_TYPE to one of the sanitizers ('asan', 'tsan' or 'ubsan') to automatically build with that sanitizer enabled
#

if("${CMAKE_BUILD_TYPE}" STREQUAL "asan")
  message(STATUS "BML: Build will have ASAN enabled!")
  set(SANITIZERS_ENABLED TRUE)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "tsan")
  message(STATUS "BML: Build will have TSAN enabled!")
  set(SANITIZERS_ENABLED TRUE)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "msan")
  message(STATUS "BML: Build will have MSAN enabled!")
  set(SANITIZERS_ENABLED TRUE)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "ubsan")
  message(STATUS "BML: Build will have UBSAN enabled!")
  set(SANITIZERS_ENABLED TRUE)
else()
  set(SANITIZERS_ENABLED FALSE)
endif()

set(SANITIZERS_ADDITIONAL_FLAGS "-Og")

# AddressSanitize
set(CMAKE_C_FLAGS_ASAN "-fsanitize=fuzzer-no-link,address,leak -fno-omit-frame-pointer -g ${SANITIZERS_ADDITIONAL_FLAGS}")
set(CMAKE_CXX_FLAGS_ASAN "-fsanitize=fuzzer-no-link,address,leak -fno-omit-frame-pointer -g ${SANITIZERS_ADDITIONAL_FLAGS}")
set(SANITIZER_TEST_OPTIONS "ASAN_OPTIONS=detect_leaks=1:check_initialization_order=1:detect_stack_use_after_return=1:strict_string_checks=1")

# ThreadSanitizer
set(CMAKE_C_FLAGS_TSAN "-fsanitize=fuzzer-no-link,thread -fno-omit-frame-pointer -g ${SANITIZERS_ADDITIONAL_FLAGS}")
set(CMAKE_CXX_FLAGS_TSAN "-fsanitize=fuzzer-no-link,thread -fno-omit-frame-pointer -g ${SANITIZERS_ADDITIONAL_FLAGS}")

# MemorySanitizer (Clang only!)
set(CMAKE_C_FLAGS_MSAN "-fsanitize=fuzzer-no-link,memory -fno-optimize-sibling-calls -fsanitize-memory-track-origins=2 -fno-omit-frame-pointer -g -O2")
set(CMAKE_CXX_FLAGS_MSAN "-fsanitize=fuzzer-no-link,memory -fno-optimize-sibling-calls -fsanitize-memory-track-origins=2 -fno-omit-frame-pointer -g -O2")

# UndefinedBehaviour
set(CMAKE_C_FLAGS_UBSAN "-fsanitize=fuzzer-no-link,undefined -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow -fno-omit-frame-pointer -g ${SANITIZERS_ADDITIONAL_FLAGS}")
set(CMAKE_CXX_FLAGS_UBSAN "-fsanitize=fuzzer-no-link,undefined -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow -fno-omit-frame-pointer -g ${SANITIZERS_ADDITIONAL_FLAGS}")
set(SANITIZER_TEST_OPTIONS "UBSAN_OPTIONS=print_stacktrace=1")

if(BML_BUILD_COVERAGE)
  message(STATUS "BML: Build will code coverag enabled!")
  add_compile_options(-fprofile-arcs -ftest-coverage --coverage)
  add_link_options(-fprofile-arcs -ftest-coverage)

  add_custom_target(CollectCoverage
    COMMAND lcov --quiet --base-directory ${PROJECT_SOURCE_DIR} --directory . --exclude ${PROJECT_BINARY_DIR} --exclude ${PROJECT_SOURCE_DIR}/test --no-external --capture --output-file Testing/coverage.info
    COMMAND genhtml --quiet Testing/coverage.info --output-directory=Testing/coverage_report
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  )

  add_custom_target(ClearCoverageCounters
    COMMAND lcov --zerocounters --directory ${PROJECT_BINARY_DIR}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  )
endif()
