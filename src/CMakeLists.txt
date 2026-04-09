add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/core)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/builtin)

target_include_directories(
  ${CANDY_TARGET_CORE}
  PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}
  ${PROJECT_BINARY_DIR}
)

set_target_properties(
  ${CANDY_TARGET_CORE}
  ${CANDY_TARGET_BUILTIN}
  PROPERTIES
  C_STANDARD 99
  C_STANDARD_REQUIRED ON
  C_EXTENSIONS OFF
  COMPILE_OPTIONS "-Wall;-Wextra;-Werror;-Wfatal-errors;-Wno-unused-parameter"
)

if (CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(${CANDY_TARGET_CORE} PRIVATE -O3 -DNDEBUG)
  target_compile_options(${CANDY_TARGET_BUILTIN} PRIVATE -O3 -DNDEBUG)
endif()
