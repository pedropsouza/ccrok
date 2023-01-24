add_library(memutil SHARED
  memutil.c
)
add_library(vec SHARED
  ivec.c
  fvec.c
)
add_library(crok SHARED
  crok/crok.c
  crok/net.c
  crok/data.c
  crok/state.c
  crok/board.c
  crok/phys.c
  crok/gloop.c
  crok/unit.c
  draw.c
  serde.c
)

target_link_libraries(crok
  crc
  ${SDL2_TTF_LIBRARIES}
)

add_library(sched SHARED
  sched.c
)

###################################################################

# COVERAGE
if (${CMAKE_BUILD_TYPE} MATCHES "Coverage")
  if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
    file(GLOB ALL_SRC_C_FILES "*.c")

    foreach(src ${ALL_SRC_C_FILES})
      get_filename_component(name ${src} NAME_WLE)
      message(STATUS "Coverage build for: ${name}")
      target_compile_options(${name} PRIVATE --coverage)
      target_link_libraries(${name} gcov)
    endforeach(src)

  else ()
    message(FATAL_ERROR "Only GCC is supported for coverage")
  endif ()
endif ()
