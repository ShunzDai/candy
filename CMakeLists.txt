cmake_minimum_required(VERSION 3.5.1)

project(candy)

set(CANDY_CONFIG_INTEGER_TYPE       int64_t)
set(CANDY_CONFIG_FLOAT_TYPE         double)
set(CANDY_CONFIG_BOOLEAN_TYPE       bool)

set(CANDY_CONFIG_MEMORY_ALIGNMENT   false)
set(CANDY_CONFIG_BUFFER_EXPAND_SIZE 4)

set(CANDY_CONFIG_LOG_LEVEL          CANDY_LOG_LEVEL_D)
set(CANDY_CONFIG_LOG_COLOR          true)

set(CANDY_TARGET_CORE        candy_core)
set(CANDY_TARGET_BUILTIN     candy_builtin)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/GetGitRevisionDescription.cmake)

git_describe(CANDY_GIT_DESCRIBE "${CMAKE_CURRENT_LIST_DIR}")

configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/cmake/candy_config.h.in
  ${PROJECT_BINARY_DIR}/candy_config.h
)

add_subdirectory(src)

if (CMAKE_PROJECT_NAME STREQUAL "candy")
  add_subdirectory(test)
  add_subdirectory(shell)
  add_subdirectory(dummy)
endif()
