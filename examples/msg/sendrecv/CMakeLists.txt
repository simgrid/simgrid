add_executable       (sendrecv      sendrecv.c)
target_link_libraries(sendrecv      simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv_CLM03.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv_Reno.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv_Vegas.tesh
  PARENT_SCOPE)
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_sendrecv.xml
  PARENT_SCOPE)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv.c
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/README
  PARENT_SCOPE)
