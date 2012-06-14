cmake_minimum_required(VERSION 2.6)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/gras.tesh
  PARENT_SCOPE
)
set(xml_files
  ${xml_files}
  PARENT_SCOPE
)
set(teshsuite_src
  ${teshsuite_src}
  PARENT_SCOPE
)
set(bin_files
  ${bin_files}
  PARENT_SCOPE
)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/README
  PARENT_SCOPE
)