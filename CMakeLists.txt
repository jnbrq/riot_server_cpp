# Canberk Sönmez

cmake_minimum_required(VERSION 3.10)
project(riot_server_cpp C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT DEFINED riot_server_cpp_NO_TESTS)
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

add_executable(
    riot_server_cpp
    src/main.cpp)

if(WIN32)
    target_link_libraries(riot_server_cpp wsock32 ws2_32)
endif()

target_link_libraries(
        riot_server_cpp
        Threads::Threads
        Boost::boost
        Boost::system
        OpenSSL::SSL
        OpenSSL::Crypto
        )

target_include_directories(
    riot_server_cpp
    PUBLIC ${riot_server_cpp_SOURCE_DIR}/include)

find_program(CMAKE_CXX_CPPCHECK NAMES cppcheck)
if (CMAKE_CXX_CPPCHECK)
    list(
        APPEND CMAKE_CXX_CPPCHECK 
            "--enable=warning"
            "--inconclusive"
            "--force" 
            "--inline-suppr"
            "--suppressions-list=${CMAKE_SOURCE_DIR}/CppCheckSuppressions.txt"
    )
endif()

if (NOT DEFINED riot_server_cpp_NO_TESTS)

# add new tests here
set(TESTS
    sfep)

foreach(test ${TESTS})
    set(exec test_${test})
    add_executable(${exec} tests/test_${test}.cpp)
    target_include_directories(
        ${exec}
        PRIVATE ${riot_server_cpp_SOURCE_DIR}/include)
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
        WORKING_DIRECTORY ${riot_server_cpp_SOURCE_DIR}/tests/test_${test}_data)
    unset(exec)
endforeach()

endif()
