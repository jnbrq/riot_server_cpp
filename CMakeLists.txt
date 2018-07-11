# Canberk Sönmez

cmake_minimum_required(VERSION 3.10)
project(RIOT_SERVER_CPP C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT DEFINED RIOT_SERVER_CPP_NO_TESTS)
enable_testing()
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

# add new tests here
set(TESTS
    sfep)

if (NOT DEFINED RIOT_SERVER_CPP_NO_TESTS)

foreach(test ${TESTS})
    set(exec test_${test})
    add_executable(${exec} tests/test_${test}.cpp)
    target_include_directories(
        ${exec}
        PRIVATE ${RIOT_SERVER_CPP_SOURCE_DIR}/include)
    target_link_libraries(
        ${exec}
        Boost::boost
        Boost::system
        Boost::unit_test_framework
        Threads::Threads
        OpenSSL::SSL
        OpenSSL::Crypto)
    target_compile_definitions(${exec} PRIVATE BOOST_TEST_DYN_LINK)
    add_test(
        NAME ${test}
        COMMAND ${exec}
        WORKING_DIRECTORY ${RIOT_SERVER_CPP_SOURCE_DIR}/tests/test_${test}_data)
    unset(exec)
endforeach()

endif()
