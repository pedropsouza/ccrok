#add_executable(
#  ccrok
#  ccrok.c
#)

add_executable(
  tchesse
  tchess.c
)

#target_link_libraries(
#  ccrok
#  memutil
#  vec
#  serde
#  net
#  crok
#  sched
#  crc
#)

target_link_libraries(
  tchesse
  memutil
  vec
  sched
  serde
  net
  tchess
)

#add_dependencies(
#  ccrok
#  resources
#)

add_dependencies(
  tchesse
  resources
)

#target_link_libraries(
#  ccrok
#  ${SDL2_LIBRARY}
#  ${SDL2_IMAGE_LIBRARIES}
#  ${SDL2_TTF_LIBRARIES}
#)

target_link_libraries(
  tchesse
  ${SDL2_LIBRARY}
  ${SDL2_IMAGE_LIBRARIES}
)
