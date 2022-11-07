file(GLOB_RECURSE SOURCES_CANDY LIST_DIRECTORIES false
  candy_stack.c
  candy_vm.c
  candy_state.c
)

add_compile_options(-Wno-implicit-fallthrough -Wno-sizeof-pointer-div)

add_library(candy_core STATIC ${SOURCES_CANDY})

target_include_directories(candy_core PUBLIC
  ..
)
