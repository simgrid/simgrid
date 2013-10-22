cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(masterslave_virtual_machines "masterslave_virtual_machines.c")
add_executable(simple_vm "simple_vm.c")
add_executable(migrate_vm "migrate_vm.c")
add_executable(bound "bound.c")
add_executable(scale "scale.c")
add_executable(multicore "multicore.c")

### Add definitions for compile
target_link_libraries(masterslave_virtual_machines simgrid)
target_link_libraries(simple_vm simgrid)
target_link_libraries(migrate_vm simgrid)
target_link_libraries(bound simgrid)
target_link_libraries(scale simgrid)
target_link_libraries(multicore simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_virtual_machines.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_virtual_machines.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/simple_plat.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/multicore_plat.xml
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_virtual_machines.c
  ${CMAKE_CURRENT_SOURCE_DIR}/simple_vm.c
  ${CMAKE_CURRENT_SOURCE_DIR}/migrate_vm.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bound.c
  ${CMAKE_CURRENT_SOURCE_DIR}/scale.c
  ${CMAKE_CURRENT_SOURCE_DIR}/multicore.c
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
