if(enable_smpi)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  endif()

  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/mc/")

  add_executable(bcbench bcbench.c)
  add_executable(mvmul mvmul.c)
  add_executable(smpi_traced tracing/smpi_traced.c)
  add_executable(smpi_traced_simple tracing/smpi_traced_simple.c)
  add_executable(smpi_replay replay/replay.c)

  target_link_libraries(bcbench simgrid)
  target_link_libraries(mvmul simgrid)
  target_link_libraries(smpi_traced simgrid)
  target_link_libraries(smpi_traced_simple simgrid)
  target_link_libraries(smpi_replay simgrid)

  if(HAVE_MC)
    add_executable(smpi_bugged1 mc/bugged1.c)
    add_executable(smpi_bugged2 mc/bugged2.c)
    add_executable(smpi_bugged1_liveness mc/bugged1_liveness.c)
    add_executable(smpi_only_send_deterministic mc/only_send_deterministic.c)
    add_executable(smpi_mutual_exclusion mc/mutual_exclusion.c)
    add_executable(smpi_non_termination1 mc/non_termination1.c)
    add_executable(smpi_non_termination2 mc/non_termination2.c)
    add_executable(smpi_non_termination3 mc/non_termination3.c)
    add_executable(smpi_non_termination4 mc/non_termination4.c)

    target_link_libraries(smpi_bugged1 simgrid)
    target_link_libraries(smpi_bugged2 simgrid)
    target_link_libraries(smpi_bugged1_liveness simgrid)
    target_link_libraries(smpi_only_send_deterministic simgrid)
    target_link_libraries(smpi_mutual_exclusion simgrid)
    target_link_libraries(smpi_non_termination1 simgrid)
    target_link_libraries(smpi_non_termination2 simgrid)
    target_link_libraries(smpi_non_termination3 simgrid)
    target_link_libraries(smpi_non_termination4 simgrid)

    set_target_properties(smpi_bugged1 PROPERTIES RUNTIME_OUTPUT_DIRECTORY "./mc")
    set_target_properties(smpi_bugged2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY "./mc")
    set_target_properties(smpi_bugged1_liveness PROPERTIES RUNTIME_OUTPUT_DIRECTORY "./mc")
    set_target_properties(smpi_only_send_deterministic PROPERTIES RUNTIME_OUTPUT_DIRECTORY "./mc")
    set_target_properties(smpi_mutual_exclusion PROPERTIES RUNTIME_OUTPUT_DIRECTORY "./mc")
    set_target_properties(smpi_non_termination1 PROPERTIES RUNTIME_OUTPUT_DIRECTORY "./mc")
    set_target_properties(smpi_non_termination2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY "./mc")
    set_target_properties(smpi_non_termination3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY "./mc")
    set_target_properties(smpi_non_termination4 PROPERTIES RUNTIME_OUTPUT_DIRECTORY "./mc")
  endif()
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/tracing/smpi_traced.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/smpi_replay.tesh
  PARENT_SCOPE)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/mvmul.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bcbench.c
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/replay.c
  ${CMAKE_CURRENT_SOURCE_DIR}/tracing/smpi_traced.c
  ${CMAKE_CURRENT_SOURCE_DIR}/tracing/smpi_traced_simple.c
  ${CMAKE_CURRENT_SOURCE_DIR}/mc/bugged2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/mc/bugged1.c
  ${CMAKE_CURRENT_SOURCE_DIR}/mc/bugged1_liveness.c
  ${CMAKE_CURRENT_SOURCE_DIR}/mc/only_send_deterministic.c
  ${CMAKE_CURRENT_SOURCE_DIR}/mc/mutual_exclusion.c
  ${CMAKE_CURRENT_SOURCE_DIR}/mc/non_termination1.c
  ${CMAKE_CURRENT_SOURCE_DIR}/mc/non_termination2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/mc/non_termination3.c
  ${CMAKE_CURRENT_SOURCE_DIR}/mc/non_termination4.c
  PARENT_SCOPE)
set(bin_files
  ${bin_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/hostfile
  ${CMAKE_CURRENT_SOURCE_DIR}/mc/promela_bugged1_liveness
  ${CMAKE_CURRENT_SOURCE_DIR}/mc/hostfile_bugged1_liveness
  ${CMAKE_CURRENT_SOURCE_DIR}/mc/hostfile_bugged1
  ${CMAKE_CURRENT_SOURCE_DIR}/mc/hostfile_bugged2
  ${CMAKE_CURRENT_SOURCE_DIR}/mc/hostfile_only_send_deterministic
  ${CMAKE_CURRENT_SOURCE_DIR}/mc/hostfile_mutual_exclusion
  ${CMAKE_CURRENT_SOURCE_DIR}/mc/hostfile_non_termination
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/actions0.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/actions1.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/actions_allReduce.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/actions_allgatherv.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/actions_alltoall.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/actions_alltoallv.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/actions_barrier.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/actions_bcast.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/actions_bcast_reduce_datatypes.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/actions_gather.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/actions_reducescatter.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/actions_waitall.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/actions_with_isend.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/split_traces
  PARENT_SCOPE)
