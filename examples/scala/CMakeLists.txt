cmake_minimum_required(VERSION 2.6)

set(tesh_files
  ${tesh_files}
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/platform.xml
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  PARENT_SCOPE
  )

