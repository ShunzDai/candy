file(GLOB_RECURSE SOURCES_SHELL LIST_DIRECTORIES false
  main.c
)

add_executable(candy ${SOURCES_SHELL})

target_link_libraries(candy
  PRIVATE
  ${CANDY_TARGET_CORE}
  ${CANDY_TARGET_BUILTIN}
)

set_target_properties(candy PROPERTIES
  C_STANDARD 11
  C_STANDARD_REQUIRED ON
  C_EXTENSIONS ON
  COMPILE_OPTIONS "-Wall;-Wextra;-Werror;-Wfatal-errors;-Wno-unused-parameter"
)
