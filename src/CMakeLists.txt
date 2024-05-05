set(CANDY_VERSION        "00.00.01")
set(CANDY_ENV            "dev")
set(CANDY_TARGET_CORE    "candy_core")
set(CANDY_TARGET_BUILTIN "candy_builtin")

set(CANDY_LEXER_EXPAND_SIZE 4)

include(${PROJECT_SOURCE_DIR}/cmake/GetGitRevisionDescription.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/candy_utility.cmake)

get_versions(${CANDY_VERSION} CANDY_VERSION_MAJOR CANDY_VERSION_MINOR CANDY_VERSION_PATCH)

git_describe(CANDY_GIT_DESCRIBE "${PROJECT_SOURCE_DIR}")

configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/candy_config.h.in
  ${PROJECT_BINARY_DIR}/candy_config.h
)

add_subdirectory(core)
add_subdirectory(builtin)

target_include_directories(${CANDY_TARGET_CORE}
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${PROJECT_BINARY_DIR}
)
