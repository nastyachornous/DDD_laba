cmake_minimum_required(VERSION 3.16)

project(ReservationDomain LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Підключення GoogleTest
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Спільна бібліотека проєкту
add_library(
    reservation_domain
    src/Model/ReservationRequest.cpp
    src/Application/Services/ReservationService.cpp
    src/Infrastructure/Persistence/MemorySession.cpp
)

target_include_directories(
    reservation_domain
    PUBLIC
    src
    src/Model
)

# 1. Доменні та прикладні тести
add_executable(
    reservation_domain_tests
    tests/ReservationDomainTests.cpp
)

target_link_libraries(
    reservation_domain_tests
    PRIVATE
    reservation_domain
    GTest::gtest_main
)

# 2. Інтеграційні тести сховища
add_executable(
    memory_session_tests
    tests/Integration/MemorySessionTests.cpp
)

target_link_libraries(
    memory_session_tests
    PRIVATE
    reservation_domain
    GTest::gtest_main
)

enable_testing()

add_test(
    NAME ReservationDomainTests
    COMMAND reservation_domain_tests
)

add_test(
    NAME MemorySessionTests
    COMMAND memory_session_tests
)