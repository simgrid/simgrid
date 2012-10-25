cmake_minimum_required(VERSION 2.6)

if(enable_smpi)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else(WIN32)
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/bin/smpicc")
  endif(WIN32)

  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

  include_directories("${CMAKE_HOME_DIRECTORY}/include/smpi")


  add_executable(bcbench bcbench.c)
  add_executable(mvmul mvmul.c)
  add_executable(smpi_traced tracing/smpi_traced.c)
  add_executable(smpi_traced_simple tracing/smpi_traced_simple.c)
  add_executable(mc_bugged1 mc_bugged1.c)
  add_executable(mc_bugged2 mc_bugged2.c)
  add_executable(smpi_replay replay/replay.c)

  target_link_libraries(bcbench m simgrid smpi )
  target_link_libraries(mvmul m simgrid smpi )
  target_link_libraries(smpi_traced m simgrid smpi )
  target_link_libraries(smpi_traced_simple m simgrid smpi )
  target_link_libraries(mc_bugged1 m simgrid smpi )
  target_link_libraries(mc_bugged2 m simgrid smpi )
  target_link_libraries(smpi_replay m simgrid smpi )

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
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/actions_bcast.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/actions0.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/actions1.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/one_trace
  ${CMAKE_CURRENT_SOURCE_DIR}/replay/split_traces
  PARENT_SCOPE
  )  