add_executable       (master_worker_vm master_worker_vm.c)
target_link_libraries(master_worker_vm simgrid)
add_executable       (simple_vm        simple_vm.c)
target_link_libraries(simple_vm        simgrid)
add_executable       (migrate_vm       migrate_vm.c)
target_link_libraries(migrate_vm       simgrid)
add_executable       (bound            bound.c)
target_link_libraries(bound            simgrid)
add_executable       (scale            scale.c)
target_link_libraries(scale            simgrid)
add_executable       (multicore        multicore.c)
target_link_libraries(multicore        simgrid)
add_executable       (two_tasks_vm     two_tasks_vm.c)
target_link_libraries(two_tasks_vm     simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/master_worker_vm.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/two_tasks_vm.tesh
   ${CMAKE_CURRENT_SOURCE_DIR}/simple_vm.tesh
  PARENT_SCOPE)
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/multicore_plat.xml
  PARENT_SCOPE)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/master_worker_vm.c
  ${CMAKE_CURRENT_SOURCE_DIR}/simple_vm.c
  ${CMAKE_CURRENT_SOURCE_DIR}/migrate_vm.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bound.c
  ${CMAKE_CURRENT_SOURCE_DIR}/scale.c
  ${CMAKE_CURRENT_SOURCE_DIR}/multicore.c
  ${CMAKE_CURRENT_SOURCE_DIR}/two_tasks_vm.c
  PARENT_SCOPE)
