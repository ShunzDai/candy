# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wfatal-errors -Werror)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(main)

include_directories(
  ../../config
)

add_subdirectory(../middleware ${CMAKE_SOURCE_DIR}/build/middleware)
add_subdirectory(../../src ${CMAKE_SOURCE_DIR}/build/candy_core)
