cmake_minimum_required(VERSION 2.6)

if(HAVE_MC)
  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test/")

  add_executable(centralized centralized_mutex.c)
  add_executable(bugged1     bugged1.c)
  add_executable(bugged2      bugged2.c)
  add_executable(bugged3	   bugged3.c)
  add_executable(random_test random_test.c)
  add_executable(bugged1_liveness bugged1_liveness.c)
  add_executable(bugged2_liveness bugged2_liveness.c)
  add_executable(chord_liveness chord_liveness.c)
  add_executable(test/snapshot_comparison_liveness1 test/snapshot_comparison_liveness1.c)
  add_executable(test/snapshot_comparison_liveness2 test/snapshot_comparison_liveness2.c)
  add_executable(test/snapshot_comparison_liveness3 test/snapshot_comparison_liveness3.c)
  add_executable(test/snapshot_comparison_liveness4 test/snapshot_comparison_liveness4.c)
  add_executable(test/snapshot_comparison_liveness5 test/snapshot_comparison_liveness5.c)

  target_link_libraries(centralized simgrid m )
  target_link_libraries(bugged1     simgrid m )
  target_link_libraries(bugged2     simgrid m )
  target_link_libraries(bugged3     simgrid m )
  target_link_libraries(random_test     simgrid m )
  target_link_libraries(bugged1_liveness     simgrid m )
  target_link_libraries(bugged2_liveness     simgrid m )
  target_link_libraries(chord_liveness     simgrid m )
  target_link_libraries(test/snapshot_comparison_liveness1     simgrid m )
  target_link_libraries(test/snapshot_comparison_liveness2     simgrid m )
  target_link_libraries(test/snapshot_comparison_liveness3     simgrid m )
  target_link_libraries(test/snapshot_comparison_liveness4     simgrid m )
  target_link_libraries(test/snapshot_comparison_liveness5     simgrid m )

endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/bugged1.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/bugged2.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/centralized.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/deploy_bugged1_liveness.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deploy_bugged1.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deploy_bugged2_liveness.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deploy_bugged2.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deploy_bugged3.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deploy_mutex.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deploy_random_test.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/platform.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deploy_chord_liveness.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/test/deploy_snapshot_comparison.xml
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/bugged1.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bugged1_liveness.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bugged2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bugged2_liveness.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bugged3.c
  ${CMAKE_CURRENT_SOURCE_DIR}/centralized_mutex.c
  ${CMAKE_CURRENT_SOURCE_DIR}/random_test.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bugged1_liveness.h
  ${CMAKE_CURRENT_SOURCE_DIR}/bugged2_liveness.h
  ${CMAKE_CURRENT_SOURCE_DIR}/chord_liveness.c
  ${CMAKE_CURRENT_SOURCE_DIR}/chord_liveness.h
  ${CMAKE_CURRENT_SOURCE_DIR}/test/snapshot_comparison_liveness1.c
  ${CMAKE_CURRENT_SOURCE_DIR}/test/snapshot_comparison_liveness2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/test/snapshot_comparison_liveness3.c
  ${CMAKE_CURRENT_SOURCE_DIR}/test/snapshot_comparison_liveness4.c
  ${CMAKE_CURRENT_SOURCE_DIR}/test/snapshot_comparison_liveness5.c
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/parse_dwarf
  ${CMAKE_CURRENT_SOURCE_DIR}/promela_bugged1_liveness
  ${CMAKE_CURRENT_SOURCE_DIR}/promela_bugged2_liveness
  ${CMAKE_CURRENT_SOURCE_DIR}/promela_chord_liveness
  ${CMAKE_CURRENT_SOURCE_DIR}/test/promela
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  PARENT_SCOPE
  )
