file(GLOB_RECURSE SOURCES_SHELL LIST_DIRECTORIES false
  main.c
)

add_executable(shell ${SOURCES_SHELL})

target_link_libraries(shell PUBLIC candy_core -lreadline)
