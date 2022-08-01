cmake_minimum_required(VERSION 3.5.1)

add_compile_options(-Wall -Wextra -Werror -Wfatal-errors -Wno-unused-parameter)

add_definitions(-DCANDY_DEBUG_MODE)

project(candy)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/config
)

add_subdirectory(src)
add_subdirectory(components)
add_subdirectory(demo)
