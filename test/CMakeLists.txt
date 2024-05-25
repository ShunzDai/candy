find_package(GTest)

file(GLOB_RECURSE SOURCES_TEST LIST_DIRECTORIES false
  test_gc.cpp
  test_exception.cpp
  test_array.cpp
  test_table.cpp
  test_lexer.cpp
  # test_parser.cpp
  # test_vm.cpp
  main.cpp
)

add_executable(test ${SOURCES_TEST})

target_link_libraries(test PUBLIC candy_core GTest::GTest)

set_target_properties(test PROPERTIES
  CXX_STANDARD 17
  C_STANDARD_REQUIRED ON
  C_EXTENSIONS OFF
  COMPILE_OPTIONS "-Wall;-Wextra;-Werror;-Wfatal-errors;-Wno-unused-parameter"
  COMPILE_DEFINITIONS "CANDY_TEST=true"
)
