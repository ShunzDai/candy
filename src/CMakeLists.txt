file(GLOB_RECURSE SOURCES_CANDY LIST_DIRECTORIES true
  *.c
)

add_library(candy_core STATIC ${SOURCES_CANDY})

target_include_directories(candy_core PUBLIC
  ..
)
