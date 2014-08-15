cmake_minimum_required (VERSION 2.6)
project (hackrf_projects)
set(EXECUTABLE_OUTPUT_PATH  ${CMAKE_BINARY_DIR})

add_subdirectory (src)
include_directories(include)
