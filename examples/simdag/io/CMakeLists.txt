add_executable       (sd_io sd_io.c)
target_link_libraries(sd_io simgrid)

ADD_TESH(simdag-io --setenv bindir=${CMAKE_BINARY_DIR}/examples/simdag/ --setenv srcdir=${CMAKE_HOME_DIRECTORY}/ --cd ${CMAKE_HOME_DIRECTORY}/examples/ ${CMAKE_HOME_DIRECTORY}/examples/simdag/io/io.tesh)

set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/io.tesh  PARENT_SCOPE)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/sd_io.c  PARENT_SCOPE)
