cmake_minimum_required(VERSION 3.16)
project(midnight_queue LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(queue_algorithms INTERFACE)
target_include_directories(queue_algorithms INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(queue_tests tests/test_algorithms.cpp)
target_link_libraries(queue_tests PRIVATE queue_algorithms)

add_executable(queue_profile src/profile.cpp)
target_link_libraries(queue_profile PRIVATE queue_algorithms)

add_executable(queue_experiments src/experiment_runner.cpp)
target_link_libraries(queue_experiments PRIVATE queue_algorithms)

enable_testing()
add_test(NAME queue_algorithms COMMAND queue_tests)
