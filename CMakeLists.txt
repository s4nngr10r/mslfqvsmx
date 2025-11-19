cmake_minimum_required(VERSION 3.14)
project(LockFreeQueue VERSION 1.0.0 LANGUAGES CXX)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
endif()

# Header-only library
add_library(lock_free_queue INTERFACE)
target_include_directories(lock_free_queue INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Enable testing
enable_testing()

# Google Test dependency
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Test executable
add_executable(lock_free_queue_tests
    tests/unit_tests.cpp
    tests/mutex_queue_test.cpp
)

target_link_libraries(lock_free_queue_tests
    PRIVATE
    lock_free_queue
    GTest::gtest_main
    pthread
)

# Add test to CTest
include(GoogleTest)
gtest_discover_tests(lock_free_queue_tests)

# Benchmark executable
add_executable(lock_free_queue_benchmark
    benchmarks/benchmark.cpp
)

target_link_libraries(lock_free_queue_benchmark
    PRIVATE
    lock_free_queue
    pthread
)
