file(GLOB_RECURSE SOURCES_SHELL LIST_DIRECTORIES false
  main.c
)

add_executable(candy ${SOURCES_SHELL})

target_link_libraries(candy PRIVATE candy_core)
