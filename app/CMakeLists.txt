add_executable(
  ccrok
  ccrok.c
)

target_link_libraries(
  ccrok
  memutil
  vec
  crok
  sched
  crc
)

add_dependencies(
  ccrok
  resources
)

target_link_libraries(
  ccrok
  ${SDL2_LIBRARY}
  ${SDL2_IMAGE_LIBRARIES}
)
