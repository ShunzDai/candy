file(GLOB_RECURSE SOURCES_CANDY LIST_DIRECTORIES true
  *.c
)

add_compile_options(-Wno-implicit-fallthrough)

add_library(candy_core STATIC ${SOURCES_CANDY})

target_include_directories(candy_core PUBLIC
  ..
)
