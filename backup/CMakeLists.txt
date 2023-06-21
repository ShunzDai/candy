file(GLOB_RECURSE SOURCES_CANDY_CPP LIST_DIRECTORIES false
  candy.cpp
)

add_library(candy_cpp STATIC ${SOURCES_CANDY_CPP})

target_link_libraries(candy_cpp PUBLIC candy_core)

target_include_directories(candy_cpp PUBLIC
  .
)
