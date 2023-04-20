file(GLOB_RECURSE SOURCES_TEST LIST_DIRECTORIES false
  # test_stack.cpp
  # test_node.cpp
  test_wrap.cpp
  # test_object.cpp
  test_lexer.cpp
  test_parser.cpp
  main.cpp
)

add_compile_options(-std=gnu++17)

add_executable(test ${SOURCES_TEST})

target_link_libraries(test PUBLIC candy_core mid_os gtest)
