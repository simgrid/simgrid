foreach(x evaluate_parse_time evaluate_get_route_time)
  add_executable       (${x} ${x}.c)
  target_link_libraries(${x} simgrid)

  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}.c)
endforeach()

set(teshsuite_src ${teshsuite_src}                                                                         PARENT_SCOPE)