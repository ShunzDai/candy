set(CANDY_VERSION "00.00.01")
set(CANDY_ENV     "dev")
set(CANDY_TARGET  "candy_core")

include(${PROJECT_SOURCE_DIR}/cmake/GetGitRevisionDescription.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/candy_release.cmake)

file(GLOB_RECURSE SOURCES_CANDY LIST_DIRECTORIES false
  candy_lib.c
  candy_wrap.c
  candy_gc.c
  candy_vector.c
  candy_array.c
  candy_table.c
  candy_exception.c
  candy_proto.c
  candy_closure.c
  candy_lexer.c
  candy_parser.c
  candy_reader.c
  candy_vm.c
  candy_state.c
  # candy_builtin.c
)

add_library(${CANDY_TARGET} STATIC ${SOURCES_CANDY})

target_include_directories(${CANDY_TARGET}
  PUBLIC
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_BINARY_DIR}
  PRIVATE
  ..
)

get_versions(${CANDY_VERSION} CANDY_VERSION_MAJOR CANDY_VERSION_MINOR CANDY_VERSION_PATCH)

target_compile_definitions(${CANDY_TARGET}
PRIVATE
CANDY_VERSION_NUM=0x${CANDY_VERSION_MAJOR}${CANDY_VERSION_MINOR}${CANDY_VERSION_PATCH}U
)

git_describe(CANDY_GIT_DESCRIBE "${PROJECT_SOURCE_DIR}")


configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/candy_release.h.in
  ${PROJECT_BINARY_DIR}/candy_release.h
)
