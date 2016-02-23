add_executable       (graphxml_usage graphxml_usage.c)
target_link_libraries(graphxml_usage simgrid )

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/graphxml_usage.tesh
  PARENT_SCOPE)
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/graph.xml
  PARENT_SCOPE)
set(testsuite_src
  ${testsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/graphxml_usage.c
  PARENT_SCOPE)
