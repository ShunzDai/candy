file(GLOB_RECURSE SOURCES_DUMMY LIST_DIRECTORIES false
dummy.cpp
)

add_executable(dummy ${SOURCES_DUMMY})

target_link_libraries(dummy PUBLIC -Wl,--whole-archive candy_core -Wl,-no-whole-archive)
