# Canberk Sönmez

cmake_minimum_required(VERSION 3.10)
project(RIOT_SERVER_CPP C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT DEFINED RIOT_SERVER_CPP_NO_TESTS)
    enable_testing()

    if (CMAKE_COMPILER_IS_GNUCC)
        set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra")
    endif()
    if (MSVC)
        set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /W4")
    endif()
endif ()

find_package(
        Boost
        1.66
        REQUIRED
        COMPONENTS
        system
        unit_test_framework
)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# a hack for MSVC, otherwise I have linking error LNK1104 regarding Boost.System
if (MSVC)
	add_definitions(-DBOOST_ALL_NO_LIB)
endif()

add_library(
    _riot_server_cpp_parsers
    STATIC
    src/parsers.cpp)
target_include_directories(
    _riot_server_cpp_parsers
    PRIVATE
    ${RIOT_SERVER_CPP_SOURCE_DIR}/include)
target_compile_definitions(_riot_server_cpp_parsers PRIVATE
    RIOT_SERVER_PARSER_SEPARATELY_COMPILED)
target_link_libraries(_riot_server_cpp_parsers PRIVATE Boost::boost)

add_library(RIOT::server_cpp INTERFACE IMPORTED)

if (WIN32)
    target_link_libraries(RIOT::server_cpp INTERFACE wsock32 ws2_32)
endif()

target_link_libraries(
    RIOT::server_cpp
    INTERFACE
    _riot_server_cpp_parsers
    Threads::Threads
    Boost::boost
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto)

target_compile_definitions(
    RIOT::server_cpp
    INTERFACE
    RIOT_SERVER_PARSER_SEPARATELY_COMPILED)

target_include_directories(
    RIOT::server_cpp
    INTERFACE
    ${RIOT_SERVER_CPP_SOURCE_DIR}/include)

add_subdirectory(examples)

if (NOT DEFINED RIOT_SERVER_CPP_NO_TESTS)
add_subdirectory(tests)
endif ()
