file(GLOB_RECURSE CANDY_SOURCES_CORE LIST_DIRECTORIES false
  candy_lib.c
  candy_wrap.c
  candy_exception.c
  candy_reader.c
  candy_memory.c
  candy_buffer.c
  candy_table.c
  candy_proto.c
  candy_closure.c
  candy_userdef.c
  candy_array.c
  candy_lexer.c
  candy_parser.c
  candy_binary.c
  candy_vm.c
  candy_gc.c
  candy_state.c
  candy.c
)

add_library(${CANDY_TARGET_CORE} ${CANDY_LIB_TYPE} ${CANDY_SOURCES_CORE})

if (UNIX AND NOT APPLE)
  target_link_libraries(${CANDY_TARGET_CORE} PUBLIC m)
endif()
