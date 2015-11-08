set(TEST_NAME ${PROJECT_NAME}_ChecksumTest)
set(TEST_TARGET ${PROJECT_NAME}_checksumtest)

add_executable(${TEST_TARGET} main.cpp)

target_link_libraries(${TEST_TARGET} fdupe)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set_property(TARGET ${TEST_TARGET} PROPERTY CXX_STANDARD 11)

# This allows the test to be executed with ctest
add_test(NAME ${TEST_NAME}
         COMMAND ${TEST_TARGET}) # this is the command that will be executed to run the test

