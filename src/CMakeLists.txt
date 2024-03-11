set(CANDY_VERSION_MAJOR 1)
set(CANDY_VERSION_MINOR 0)
set(CANDY_VERSION_PATCH 0)
set(CANDY_ENV "dev")

include(${PROJECT_SOURCE_DIR}/cmake/GetGitRevisionDescription.cmake)

git_describe(CANDY_GIT_DESCRIBE "${PROJECT_SOURCE_DIR}")

file(GLOB_RECURSE SOURCES_CANDY LIST_DIRECTORIES false
  candy_lib.c
  candy_io.c
  candy_wrap.c
  candy_table.c
  candy_lexer.c
  candy_parser.c
  candy_reader.c
  candy_block.c
  candy_vm.c
  candy_state.c
  candy_builtin.c
)

add_library(candy_core STATIC ${SOURCES_CANDY})

target_include_directories(candy_core
  PUBLIC
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_BINARY_DIR}
  PRIVATE
  ..
)

configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/candy_release.in
  ${PROJECT_BINARY_DIR}/candy_release.h
)
