file(GLOB_RECURSE SOURCES_TEST LIST_DIRECTORIES false
  test_io.cpp
  test_wrap.cpp
  test_table.cpp
  test_lexer.cpp
  test_parser.cpp
  test_vm.cpp
  main.cpp
)

add_executable(test ${SOURCES_TEST})

target_link_libraries(test PUBLIC candy_core gtest)
