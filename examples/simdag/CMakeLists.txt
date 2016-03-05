foreach(x test test2 avail fail typed_tasks comm_throttling)
  add_executable       (sd_${x}     sd_${x}.c)
  target_link_libraries(sd_${x}     simgrid)
  set(examples_src ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/sd_${x}.c)
  set(tesh_files   ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/sd_${x}.tesh)

  ADD_TESH(simdag-${x} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/simdag --cd ${CMAKE_BINARY_DIR}/examples/simdag ${CMAKE_HOME_DIRECTORY}/examples/simdag/sd_${x}.tesh)
endforeach()

set(examples_src ${examples_src} PARENT_SCOPE)
set(tesh_files   ${tesh_files}   PARENT_SCOPE)
set(bin_files ${bin_files} ${CMAKE_CURRENT_SOURCE_DIR}/platform_script.lua PARENT_SCOPE)
