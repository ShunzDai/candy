file(GLOB_RECURSE SOURCES_SHELL LIST_DIRECTORIES false
  main.cpp
)

add_executable(main ${SOURCES_SHELL})

target_link_libraries(main PUBLIC candy_cpp mid_os)
