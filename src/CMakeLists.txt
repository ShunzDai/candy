file(GLOB_RECURSE SOURCES_CANDY LIST_DIRECTORIES false
  candy_lib.c
  candy_io.c
  candy_wrap.c
  candy_table.c
  candy_lexer.c
  candy_parser.c
  candy_reader.c
  candy_block.c
  candy_vm.c
  candy_state.c
  candy_builtin.c
)

add_library(candy_core STATIC ${SOURCES_CANDY})

target_include_directories(candy_core PUBLIC
  ..
)
