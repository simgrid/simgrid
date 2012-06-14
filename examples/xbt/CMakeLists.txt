cmake_minimum_required(VERSION 2.6)

set(tesh_files
  ${tesh_files}
  PARENT_SCOPE
)
set(xml_files
  ${xml_files}
  PARENT_SCOPE
)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/sem_basic.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sem_sched.c
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