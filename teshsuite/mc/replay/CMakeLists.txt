add_executable       (random_bug random_bug.c)
target_link_libraries(random_bug simgrid)

set(tesh_files     ${tesh_files}     ${CMAKE_CURRENT_SOURCE_DIR}/random_bug.tesh
                                     ${CMAKE_CURRENT_SOURCE_DIR}/random_bug_replay.tesh  PARENT_SCOPE)
set(testsuite_src  ${testsuite_src}  ${CMAKE_CURRENT_SOURCE_DIR}/random_bug.c            PARENT_SCOPE)
set(xml_files      ${xml_files}      ${CMAKE_CURRENT_SOURCE_DIR}/random_bug.xml          PARENT_SCOPE)
