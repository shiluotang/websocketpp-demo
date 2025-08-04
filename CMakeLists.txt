cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

project(websocketpp-demo VERSION 1.0)

if (WIN32)
set(Boost_ROOT $ENV{HOMEDRIVE}/local/boost_1_72_0)
endif()

find_package(GTest REQUIRED)
find_package(OpenSSL REQUIRED)
if (CMAKE_VERSION VERSION_LESS 3.30)
find_package(Boost REQUIRED COMPONENTS chrono system)
else()
find_package(Boost CONFIG REQUIRED COMPONENTS system)
endif()
find_package(websocketpp CONFIG REQUIRED)

add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/bigobj>)

enable_testing()
add_subdirectory(tests)
