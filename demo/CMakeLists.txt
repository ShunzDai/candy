file(GLOB_RECURSE SOURCES_TEST LIST_DIRECTORIES false
  test_main.cpp
  # test_queue.cpp
  test_wrap.cpp
  test_object.cpp
  test_parser.cpp
)

add_executable(${CMAKE_PROJECT_NAME}_test ${SOURCES_TEST})

target_link_libraries(${CMAKE_PROJECT_NAME}_test PUBLIC ${CMAKE_PROJECT_NAME}_core gtest)
