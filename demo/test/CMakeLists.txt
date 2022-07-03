file(GLOB_RECURSE SOURCES_TEST LIST_DIRECTORIES false
  test_main.cpp
  test_queue.cpp
  test_wrap.cpp
  test_object.cpp
  test_lexer.cpp
  test_parser.cpp
)

add_executable(test ${SOURCES_TEST})

target_include_directories(test PUBLIC
  ${CMAKE_SOURCE_DIR}/demo/middleware
  ${CMAKE_SOURCE_DIR}/module/googletest/googletest/include
)

target_link_libraries(test PUBLIC -Wl,--whole-archive candy_core mid_os -Wl,-no-whole-archive gtest)
