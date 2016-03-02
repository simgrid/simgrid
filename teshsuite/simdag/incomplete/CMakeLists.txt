add_executable       (incomplete incomplete.c)
target_link_libraries(incomplete simgrid)

set(tesh_files     ${tesh_files}     ${CMAKE_CURRENT_SOURCE_DIR}/incomplete.tesh  PARENT_SCOPE)
set(teshsuite_src  ${teshsuite_src}  ${CMAKE_CURRENT_SOURCE_DIR}/incomplete.c     PARENT_SCOPE)

ADD_TESH(tesh-simdag-incomplete --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simdag/incomplete --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/incomplete incomplete.tesh)

