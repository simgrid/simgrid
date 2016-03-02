foreach(x surf_usage surf_usage2)
  add_executable       (${x} ${x}.cpp)
  target_link_libraries(${x} simgrid)

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}.cpp)

  ADD_TESH(tesh-${x} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/surf/surf_usage --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/surf/surf_usage/${x}.tesh)
endforeach()

set(teshsuite_src ${teshsuite_src}                                               PARENT_SCOPE)
set(tesh_files    ${tesh_files}                                                  PARENT_SCOPE)

