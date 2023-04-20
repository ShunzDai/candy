file(GLOB_RECURSE SOURCES_SHELL LIST_DIRECTORIES false
  main.cpp
)

add_executable(shell ${SOURCES_SHELL})

target_link_libraries(shell PUBLIC candy_cpp mid_os)
