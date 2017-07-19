if(enable_smpi)
  set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/mc/")

  foreach(x replay trace trace_simple trace_call_location energy)
    add_executable       (smpi_${x} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)
    target_link_libraries(smpi_${x} simgrid)
    set_target_properties(smpi_${x} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
    set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)
  endforeach()

  set_target_properties(smpi_trace_call_location PROPERTIES COMPILE_FLAGS "-trace-call-location")

  foreach(x bugged1 bugged2 bugged1_liveness only_send_deterministic mutual_exclusion non_termination1 
            non_termination2 non_termination3 non_termination4)
    if(SIMGRID_HAVE_MC)
      add_executable       (smpi_${x} ${CMAKE_CURRENT_SOURCE_DIR}/mc/${x}.c)
      target_link_libraries(smpi_${x} simgrid)
      set_target_properties(smpi_${x} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/mc)
    endif()
    set(examples_src  ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/mc/${x}.c)
  endforeach()
endif()

set(examples_src  ${examples_src}                                                                          PARENT_SCOPE)
set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/energy/energy.tesh
                                   ${CMAKE_CURRENT_SOURCE_DIR}/trace/trace.tesh
                                   ${CMAKE_CURRENT_SOURCE_DIR}/trace_simple/trace_simple.tesh
                                   ${CMAKE_CURRENT_SOURCE_DIR}/trace_call_location/trace_call_location.tesh
                                   ${CMAKE_CURRENT_SOURCE_DIR}/replay/replay.tesh                          PARENT_SCOPE)
set(bin_files     ${bin_files}     ${CMAKE_CURRENT_SOURCE_DIR}/hostfile
                                   ${CMAKE_CURRENT_SOURCE_DIR}/energy/hostfile
                                   ${CMAKE_CURRENT_SOURCE_DIR}/mc/promela_bugged1_liveness
                                   ${CMAKE_CURRENT_SOURCE_DIR}/mc/hostfile_bugged1_liveness
                                   ${CMAKE_CURRENT_SOURCE_DIR}/mc/hostfile_bugged1
                                   ${CMAKE_CURRENT_SOURCE_DIR}/mc/hostfile_bugged2
                                   ${CMAKE_CURRENT_SOURCE_DIR}/mc/hostfile_only_send_deterministic
                                   ${CMAKE_CURRENT_SOURCE_DIR}/mc/hostfile_mutual_exclusion
                                   ${CMAKE_CURRENT_SOURCE_DIR}/mc/hostfile_non_termination                 PARENT_SCOPE)
set(txt_files     ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/replay/actions0.txt
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
                                   ${CMAKE_CURRENT_SOURCE_DIR}/replay/split_traces                         PARENT_SCOPE)

if(enable_smpi)
  if(SIMGRID_HAVE_MC)
    ADD_TESH(smpi-mc-only-send-determinism --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/smpi/mc --cd ${CMAKE_BINARY_DIR}/examples/smpi/mc ${CMAKE_HOME_DIRECTORY}/examples/smpi/mc/only_send_deterministic.tesh)
  endif()

  ADD_TESH(smpi-tracing        --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/smpi --cd ${CMAKE_BINARY_DIR}/examples/smpi/trace ${CMAKE_HOME_DIRECTORY}/examples/smpi/trace/trace.tesh)
  ADD_TESH(smpi-tracing-simple --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/smpi --cd ${CMAKE_BINARY_DIR}/examples/smpi/trace_simple ${CMAKE_HOME_DIRECTORY}/examples/smpi/trace_simple/trace_simple.tesh)
  ADD_TESH(smpi-tracing-call-location --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/smpi --cd ${CMAKE_BINARY_DIR}/examples/smpi/trace_call_location ${CMAKE_HOME_DIRECTORY}/examples/smpi/trace_call_location/trace_call_location.tesh)
  ADD_TESH(smpi-replay         --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/smpi --cd ${CMAKE_BINARY_DIR}/examples/smpi ${CMAKE_HOME_DIRECTORY}/examples/smpi/replay/replay.tesh)
  ADD_TESH_FACTORIES(smpi-energy "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/smpi/energy --cd ${CMAKE_BINARY_DIR}/examples/smpi/energy ${CMAKE_HOME_DIRECTORY}/examples/smpi/energy/energy.tesh)
endif()
