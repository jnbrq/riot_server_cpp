set(EXAMPLES
    example00
    example01
    example02
    example03)

foreach(EXAMPLE ${EXAMPLES})
    add_executable(${EXAMPLE} ${EXAMPLE}.cpp)
    target_link_libraries(${EXAMPLE} RIOT::server_cpp)
endforeach()
