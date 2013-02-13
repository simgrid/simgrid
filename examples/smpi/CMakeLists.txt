cmake_minimum_required(VERSION 2.6)

if(enable_smpi)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  endif()

  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

  include_directories("${CMAKE_HOME_DIRECTORY}/include/smpi")


  add_executable(bcbench bcbench.c)
  add_executable(mvmul mvmul.c)
  add_executable(smpi_traced tracing/smpi_traced.c)
  add_executable(smpi_traced_simple tracing/smpi_traced_simple.c)
  add_executable(mc_bugged1 mc_bugged1.c)
  add_executable(mc_bugged2 mc_bugged2.c)
  add_executable(smpi_replay replay/replay.c)

  target_link_libraries(bcbench simgrid smpi )
  target_link_libraries(mvmul simgrid smpi )
  target_link_libraries(smpi_traced simgrid smpi )
  target_link_libraries(smpi_traced_simple simgrid smpi )
  target_link_libraries(mc_bugged1 simgrid smpi )
  target_link_libraries(mc_bugged2 simgrid smpi )
  target_link_libraries(smpi_replay simgrid smpi )

endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/tracing/smpi_traced.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/smpi_replay.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/replay_platform.xml
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/mc_bugged2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/mc_bugged1.c
  ${CMAKE_CURRENT_SOURCE_DIR}/mvmul.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bcbench.c
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/replay.c
  ${CMAKE_CURRENT_SOURCE_DIR}/tracing/smpi_traced.c
  ${CMAKE_CURRENT_SOURCE_DIR}/tracing/smpi_traced_simple.c
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/hostfile
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/actions0.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/actions1.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/actions_allReduce.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/actions_alltoall.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/actions_alltoallv.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/actions_barrier.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/actions_bcast.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/actions_bcast_reduce_datatypes.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/actions_waitall.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/actions_with_isend.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/split_traces
  PARENT_SCOPE
  )  
