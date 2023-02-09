#add_executable(
#  ccrok
#  ccrok.c
#)

add_executable(
  solre
  solr.c
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
  solre
  memutil
  vec
  sched
  serde
  net
  solr
)

#add_dependencies(
#  ccrok
#  resources
#)

add_dependencies(
  solre
  resources
)

#target_link_libraries(
#  ccrok
#  ${SDL2_LIBRARY}
#  ${SDL2_IMAGE_LIBRARIES}
#  ${SDL2_TTF_LIBRARIES}
#)

target_link_libraries(
  solre
  ${SDL2_LIBRARY}
  ${SDL2_IMAGE_LIBRARIES}
)
