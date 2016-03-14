add_executable       (with_mutex_handling    mutex_handling.c)
target_link_libraries(with_mutex_handling    simgrid)

add_executable       (without_mutex_handling mutex_handling.c)
target_link_libraries(without_mutex_handling simgrid)
set_target_properties(without_mutex_handling PROPERTIES COMPILE_FLAGS -DDISABLE_THE_MUTEX=1)

set(tesh_files     ${tesh_files}     ${CMAKE_CURRENT_SOURCE_DIR}/with_mutex_handling.tesh
                                     ${CMAKE_CURRENT_SOURCE_DIR}/without_mutex_handling.tesh    PARENT_SCOPE)
set(testsuite_src  ${testsuite_src}  ${CMAKE_CURRENT_SOURCE_DIR}/mutex_handling.c               PARENT_SCOPE)
set(xml_files      ${xml_files}      ${CMAKE_CURRENT_SOURCE_DIR}/mutex_handling.xml             PARENT_SCOPE)
