cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

project(websocketpp-demo VERSION 1.0)

find_package(GTest REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(websocketpp CONFIG REQUIRED)

enable_testing()
add_subdirectory(tests)
