file(GLOB_RECURSE SOURCES_MID_OS LIST_DIRECTORIES false
  mid_os.cpp
)

add_library(mid_os STATIC ${SOURCES_MID_OS})

if(__linux__)
  target_link_libraries(mid_os PRIVATE pthread)
elseif(ESP_PLATFORM)
  # todo: the ESP_PLATFORM macro is lost in the esp-idf v4.4.1 2nd project due to unknown reasons
  add_definitions(-DESP_PLATFORM)
  target_link_libraries(mid_os PRIVATE idf::freertos)
endif()

target_include_directories(mid_os PUBLIC
  .
)
