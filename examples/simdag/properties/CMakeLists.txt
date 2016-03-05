add_executable       (sd_prop sd_prop.c)
target_link_libraries(sd_prop simgrid)

ADD_TESH(simdag-prop --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/simdag --cd ${CMAKE_BINARY_DIR}/examples/simdag ${CMAKE_HOME_DIRECTORY}/examples/simdag/properties/test_prop.tesh)

set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/test_prop.tesh PARENT_SCOPE)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/sd_prop.c      PARENT_SCOPE)
