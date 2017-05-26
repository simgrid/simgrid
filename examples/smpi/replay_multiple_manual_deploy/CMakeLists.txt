if(enable_smpi)
  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  add_executable       (replay_multiple_manual replay_multiple_manual.cpp)
  target_link_libraries(replay_multiple_manual simgrid)
  IF(NOT HAVE_MC)
    ADD_TESH(smpi-replay-multiple-manual --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple_manual_deploy --cd ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy ${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple_manual_deploy/replay_multiple_manual.tesh)
  ENDIF()
endif()

set(txt_files     ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/generate_multiple_deployment.sh
                                   ${CMAKE_CURRENT_SOURCE_DIR}/compute_only.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/compute_only/actions0.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/compute_only/actions1.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/empty.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/empty/actions0.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/empty/actions1.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/mixed.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/mixed/actions0.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/mixed/actions1.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/replay_multiple_manual.tesh
                                   ${CMAKE_CURRENT_SOURCE_DIR}/workload_compute
                                   ${CMAKE_CURRENT_SOURCE_DIR}/workload_compute_consecutive
                                   ${CMAKE_CURRENT_SOURCE_DIR}/workload_compute_consecutive2
                                   ${CMAKE_CURRENT_SOURCE_DIR}/workload_compute_simple
                                   ${CMAKE_CURRENT_SOURCE_DIR}/workload_empty1
                                   ${CMAKE_CURRENT_SOURCE_DIR}/workload_empty2
                                   ${CMAKE_CURRENT_SOURCE_DIR}/workload_empty2_same_resources
                                   ${CMAKE_CURRENT_SOURCE_DIR}/workload_empty2_same_time
                                   ${CMAKE_CURRENT_SOURCE_DIR}/workload_empty2_same_time_and_resources
                                   ${CMAKE_CURRENT_SOURCE_DIR}/workload_mixed1
                                   ${CMAKE_CURRENT_SOURCE_DIR}/workload_mixed2
                                   ${CMAKE_CURRENT_SOURCE_DIR}/workload_mixed2_same_resources
                                   ${CMAKE_CURRENT_SOURCE_DIR}/workload_mixed2_same_time
                                   ${CMAKE_CURRENT_SOURCE_DIR}/workload_mixed2_same_time_and_resources
                                   ${CMAKE_CURRENT_SOURCE_DIR}/workload_no_job PARENT_SCOPE)
set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/replay_multiple_manual.tesh           PARENT_SCOPE)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/replay_multiple_manual.cpp              PARENT_SCOPE)
