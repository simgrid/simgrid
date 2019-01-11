if(enable_smpi)
  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  add_executable       (replay_multiple_manual EXCLUDE_FROM_ALL replay_multiple_manual.cpp)
  target_link_libraries(replay_multiple_manual simgrid)
  add_dependencies(tests replay_multiple_manual)

  # Define a list of a tesh files
  list(APPEND tesh_filename_list replay_multiple_manual_nojob)
  list(APPEND tesh_filename_list replay_multiple_manual_coll1)
  list(APPEND tesh_filename_list replay_multiple_manual_coll2_st_sr_noise)
  list(APPEND tesh_filename_list replay_multiple_manual_empty1)
  list(APPEND tesh_filename_list replay_multiple_manual_empty2)
  list(APPEND tesh_filename_list replay_multiple_manual_mixed1)
  list(APPEND tesh_filename_list replay_multiple_manual_mixed2)
  list(APPEND tesh_filename_list replay_multiple_manual_mixed2_st)
  list(APPEND tesh_filename_list replay_multiple_manual_mixed2_st_noise)
  list(APPEND tesh_filename_list replay_multiple_manual_mixed2_st_sr)
  list(APPEND tesh_filename_list replay_multiple_manual_mixed2_st_sr_noise)

  IF(NOT HAVE_MC)
    foreach(rm_tesh_file IN LISTS tesh_filename_list)
      string(REPLACE "_" "-" dash_separated ${rm_tesh_file})
      ADD_TESH("smpi-${dash_separated}"
        --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple_manual_deploy
        --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
        --cd ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy
        ${CMAKE_CURRENT_SOURCE_DIR}/${rm_tesh_file}.tesh)
    endforeach()
  ENDIF()
endif()

set(txt_files     ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/compute_only.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/compute_only/actions0.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/compute_only/actions1.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/coll.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/coll/actions0.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/coll/actions1.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/empty.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/empty/actions0.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/empty/actions1.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/mixed.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/mixed/actions0.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/mixed/actions1.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/workload_compute
                                   ${CMAKE_CURRENT_SOURCE_DIR}/workload_compute_consecutive
                                   ${CMAKE_CURRENT_SOURCE_DIR}/workload_compute_consecutive2
                                   ${CMAKE_CURRENT_SOURCE_DIR}/workload_compute_simple
                                   ${CMAKE_CURRENT_SOURCE_DIR}/workload_mixed2_same_time
                                   ${CMAKE_CURRENT_SOURCE_DIR}/workload_mixed2_same_time_and_resources PARENT_SCOPE)
set(tesh_files  ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/replay_multiple_manual_nojob.tesh
                                ${CMAKE_CURRENT_SOURCE_DIR}/replay_multiple_manual_coll1.tesh
                                ${CMAKE_CURRENT_SOURCE_DIR}/replay_multiple_manual_coll2_st_sr_noise.tesh
                                ${CMAKE_CURRENT_SOURCE_DIR}/replay_multiple_manual_empty1.tesh
                                ${CMAKE_CURRENT_SOURCE_DIR}/replay_multiple_manual_empty2.tesh
                                ${CMAKE_CURRENT_SOURCE_DIR}/replay_multiple_manual_mixed1.tesh
                                ${CMAKE_CURRENT_SOURCE_DIR}/replay_multiple_manual_mixed2.tesh
                                ${CMAKE_CURRENT_SOURCE_DIR}/replay_multiple_manual_mixed2_st.tesh
                                ${CMAKE_CURRENT_SOURCE_DIR}/replay_multiple_manual_mixed2_st_noise.tesh
                                ${CMAKE_CURRENT_SOURCE_DIR}/replay_multiple_manual_mixed2_st_sr.tesh
                                ${CMAKE_CURRENT_SOURCE_DIR}/replay_multiple_manual_mixed2_st_sr_noise.tesh PARENT_SCOPE)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/replay_multiple_manual.cpp PARENT_SCOPE)
