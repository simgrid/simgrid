cmake_minimum_required(VERSION 2.6)

if(HAVE_MC)
  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test/")
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/chord/")

  add_executable(centralized centralized_mutex.c)
  add_executable(bugged1     bugged1.c)
  add_executable(bugged2      bugged2.c)
  add_executable(bugged3	   bugged3.c)
  add_executable(random_test random_test.c)
  add_executable(bugged1_liveness bugged1_liveness.c)
  add_executable(bugged2_liveness bugged2_liveness.c)
  add_executable(chord/chord_liveness chord/chord_liveness.c)
  add_executable(test/snapshot_comparison1 test/snapshot_comparison1.c)
  add_executable(test/snapshot_comparison2 test/snapshot_comparison2.c)
  add_executable(test/snapshot_comparison3 test/snapshot_comparison3.c)
  add_executable(test/snapshot_comparison4 test/snapshot_comparison4.c)
  add_executable(test/snapshot_comparison5 test/snapshot_comparison5.c)

  target_link_libraries(centralized simgrid )
  target_link_libraries(bugged1     simgrid )
  target_link_libraries(bugged2     simgrid )
  target_link_libraries(bugged3     simgrid )
  target_link_libraries(random_test     simgrid )
  target_link_libraries(bugged1_liveness     simgrid )
  target_link_libraries(bugged2_liveness     simgrid )
  target_link_libraries(chord/chord_liveness     simgrid )
  target_link_libraries(test/snapshot_comparison1     simgrid )
  target_link_libraries(test/snapshot_comparison2     simgrid )
  target_link_libraries(test/snapshot_comparison3     simgrid )
  target_link_libraries(test/snapshot_comparison4     simgrid )
  target_link_libraries(test/snapshot_comparison5     simgrid )

endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/bugged1.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/bugged1_liveness.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/centralized.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/chord/chord_neverjoin.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test/snapshot_comparison1.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test/snapshot_comparison2.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test/snapshot_comparison3.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test/snapshot_comparison4.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test/snapshot_comparison5.tesh
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
  ${CMAKE_CURRENT_SOURCE_DIR}/chord/deploy_chord_liveness.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/test/deploy_snapshot_comparison.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/test/snapshot_comparison_platform.xml
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
  ${CMAKE_CURRENT_SOURCE_DIR}/chord/chord_liveness.c
  ${CMAKE_CURRENT_SOURCE_DIR}/chord/chord_liveness.h
  ${CMAKE_CURRENT_SOURCE_DIR}/test/snapshot_comparison1.c
  ${CMAKE_CURRENT_SOURCE_DIR}/test/snapshot_comparison2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/test/snapshot_comparison3.c
  ${CMAKE_CURRENT_SOURCE_DIR}/test/snapshot_comparison4.c
  ${CMAKE_CURRENT_SOURCE_DIR}/test/snapshot_comparison5.c
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/parse_dwarf
  ${CMAKE_CURRENT_SOURCE_DIR}/promela_bugged1_liveness
  ${CMAKE_CURRENT_SOURCE_DIR}/promela_bugged2_liveness
  ${CMAKE_CURRENT_SOURCE_DIR}/chord/promela_chord_liveness
  ${CMAKE_CURRENT_SOURCE_DIR}/test/promela
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  PARENT_SCOPE
  )
