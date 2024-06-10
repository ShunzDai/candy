file(GLOB_RECURSE CANDY_SOURCES_CORE LIST_DIRECTORIES false
  candy_lib.c
  candy_wrap.c
  candy_gc.c
  candy_vector.c
  candy_array.c
  candy_table.c
  candy_userdef.c
  candy_exception.c
  candy_proto.c
  candy_closure.c
  candy_print.c
  candy_buffer.c
  candy_lexer.c
  candy_parser.c
  candy_reader.c
  candy_vm.c
  candy_state.c
)

add_library(${CANDY_TARGET_CORE} SHARED ${CANDY_SOURCES_CORE})
