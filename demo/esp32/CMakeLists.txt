# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wfatal-errors -Werror)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++17")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(main)

include_directories(
  ../../config
)

add_definitions(-DCANDY_DEBUG_MODE)

add_subdirectory(../../components/middleware/mid_os ${CMAKE_SOURCE_DIR}/build/middleware/mid_os)
add_subdirectory(../../src ${CMAKE_SOURCE_DIR}/build/candy_core)
add_subdirectory(../../cpp ${CMAKE_SOURCE_DIR}/build/candy_cpp)
