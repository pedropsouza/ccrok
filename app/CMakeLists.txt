add_executable(
  ccrok
  ccrok.c
)

target_link_libraries(
  ccrok
  memutil
  fvec
  crok
  sched
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
