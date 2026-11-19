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
  # Link-time optimization lets the hot helpers inline across translation units.
  target_compile_options(${CANDY_TARGET_CORE} PRIVATE -O3 -DNDEBUG -flto
    -falign-functions=32 -falign-loops=32)
  target_compile_options(${CANDY_TARGET_BUILTIN} PRIVATE -O3 -DNDEBUG -flto
    -falign-functions=32 -falign-loops=32)
  set_property(TARGET ${CANDY_TARGET_CORE} APPEND_STRING PROPERTY LINK_FLAGS " -flto")
  set_property(TARGET ${CANDY_TARGET_BUILTIN} APPEND_STRING PROPERTY LINK_FLAGS " -flto")
endif()

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
  # GCC diagnoses cross-translation-unit always_inline before LTO sees the
  # callers. Keep the attribute on definitions, then let LTO enforce it.
  target_compile_options(${CANDY_TARGET_CORE} PRIVATE -Wno-error=attributes)
  if (NOT CANDY_STATIC)
    target_compile_options(${CANDY_TARGET_CORE} PRIVATE -fno-semantic-interposition)
  endif()
endif()
