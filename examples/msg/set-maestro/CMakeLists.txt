add_executable       (set-maestro set-maestro.c)
target_link_libraries(set-maestro simgrid)

set(tesh_files   ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/set-maestro.tesh  PARENT_SCOPE)
set(xml_files    ${xml_files}    ${CMAKE_CURRENT_SOURCE_DIR}/set-maestro_d.xml PARENT_SCOPE)
set(examples_src ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/set-maestro.c     PARENT_SCOPE)
