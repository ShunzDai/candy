file(GLOB_RECURSE SOURCES_DUMMY LIST_DIRECTORIES false
  candy.cpp
  main.cpp
)

add_executable(dummy ${SOURCES_DUMMY})

target_link_libraries(dummy
  PRIVATE
  ${CANDY_TARGET_CORE}
  ${CANDY_TARGET_BUILTIN}
)

target_compile_options(dummy PRIVATE -std=c++17)
