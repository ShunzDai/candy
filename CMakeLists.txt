cmake_minimum_required(VERSION 3.5.1)

add_compile_options(-Wall -Wextra -Werror -Wfatal-errors -Wno-unused-parameter)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++17")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

project(candy)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/config
)

add_subdirectory(components)
add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(shell)
add_subdirectory(dummy)
