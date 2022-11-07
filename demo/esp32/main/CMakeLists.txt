idf_component_register(
  SRCS
  "main.cpp"
  INCLUDE_DIRS
  "."
)

target_link_libraries(${COMPONENT_LIB} PUBLIC -Wl,--whole-archive candy_cpp -Wl,-no-whole-archive mid_os)
