foreach(x 0 1 2 3 4 5 6)
  add_executable       (basic${x} basic${x}.c)
  target_link_libraries(basic${x} simgrid)

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/basic${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/basic${x}.c)

  ADD_TESH(tesh-simdag-basic${x} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simdag/basic --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/basic basic${x}.tesh)
endforeach()

set(teshsuite_src ${teshsuite_src}                                               PARENT_SCOPE)
set(tesh_files    ${tesh_files}                                                  PARENT_SCOPE)
