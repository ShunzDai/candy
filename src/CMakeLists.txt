file(GLOB_RECURSE SOURCES_CANDY LIST_DIRECTORIES false
  candy_queue.c
  candy_object.c
  candy_stack.c
  candy_io.c
  candy_lexer.c
  candy_vm.c
  candy_state.c
  candy_platform.c
)

add_compile_options(-Wno-implicit-fallthrough -Wno-sizeof-pointer-div)

add_library(candy_core STATIC ${SOURCES_CANDY})

target_include_directories(candy_core PUBLIC
  ..
)
