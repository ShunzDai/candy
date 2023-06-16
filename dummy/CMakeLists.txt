file(GLOB_RECURSE SOURCES_DUMMY LIST_DIRECTORIES false
  main.cpp
)

add_executable(dummy ${SOURCES_DUMMY})

target_link_libraries(dummy PUBLIC candy_cpp)
