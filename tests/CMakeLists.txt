# add new tests here
set(TESTS
    sfep
    mpl)

add_library(RIOT::test_cpp INTERFACE IMPORTED)
target_link_libraries(
    RIOT::test_cpp
    INTERFACE
    RIOT::server_cpp
    Boost::unit_test_framework)
target_compile_definitions(
    RIOT::test_cpp
    INTERFACE
    BOOST_TEST_DYN_LINK)

foreach(test ${TESTS})
    set(exec test_${test})
    add_executable(
        ${exec}
        ${RIOT_SERVER_CPP_SOURCE_DIR}/tests/test_${test}.cpp)
    target_link_libraries(${exec} RIOT::test_cpp)
    add_test(
        NAME ${test}
        COMMAND ${exec}
        WORKING_DIRECTORY
            ${RIOT_SERVER_CPP_SOURCE_DIR}/tests/test_${test}_data)
    unset(exec)
endforeach()
