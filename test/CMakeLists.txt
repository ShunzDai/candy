file(GLOB_RECURSE SOURCES_TEST LIST_DIRECTORIES false
  test_main.cpp
  test_queue.cpp
  # test_wrap.cpp
  # test_object.cpp
  test_lexer.cpp
  # test_parser.cpp
)

add_compile_options(-std=gnu++17)

add_executable(test ${SOURCES_TEST})

target_link_libraries(test PUBLIC -Wl,--whole-archive candy_core -Wl,-no-whole-archive gtest)
