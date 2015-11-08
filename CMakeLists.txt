
cmake_minimum_required(VERSION 2.8.0)

set(PROJECT_NAME libfdupe)
project(${PROJECT_NAME})

set(${PROJECT_NAME}_VERSION_MAJOR 0)
set(${PROJECT_NAME}_VERSION_MINOR 1)

include_directories(.)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_definitions(-D_GLIBCXX_USE_CXX11_ABI=1)

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(tools)
