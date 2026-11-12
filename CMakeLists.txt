
cmake_minimum_required(VERSION 3.16)

project(ReservationDomain LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(
    reservation_domain
    src/Model/ReservationRequest.cpp
    src/Application/Services/ReservationService.cpp
)

target_include_directories(
    reservation_domain
    PUBLIC
    src/Model
)

add_executable(
    reservation_domain_tests
    tests/ReservationDomainTests.cpp
)

target_link_libraries(
    reservation_domain_tests
    PRIVATE
    reservation_domain
)

enable_testing()

add_test(
    NAME ReservationDomainTests
    COMMAND reservation_domain_tests
)
