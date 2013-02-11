cmake_minimum_required(VERSION 2.6)

if(enable_smpi)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "sh ${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  endif()
  
  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
  
  include_directories("${CMAKE_HOME_DIRECTORY}/include/smpi")

  add_executable(alltoall2 alltoall2.c )
  add_executable(alltoall_basic alltoall_basic.c)
  add_executable(alltoallv alltoallv.c)
  add_executable(allreduce allreduce.c)
  add_executable(bcast bcast.c)
  add_executable(compute compute.c)
  add_executable(compute2 compute2.c)
  add_executable(compute3 compute3.c)
  add_executable(pingpong pingpong.c)
  add_executable(scatter scatter.c)
  add_executable(reduce reduce.c)
  add_executable(split split.c)
  add_executable(smpi_sendrecv sendrecv.c)
  add_executable(ttest01 ttest01.c)
  add_executable(vector_test vector_test.c)
  add_executable(hvector_test hvector_test.c)
  add_executable(indexed_test indexed_test.c)
  add_executable(struct_test struct_test.c)

  target_link_libraries(alltoall2 simgrid smpi )
  target_link_libraries(alltoall_basic simgrid smpi )
  target_link_libraries(alltoallv simgrid smpi )
  target_link_libraries(allreduce simgrid smpi )
  target_link_libraries(bcast simgrid smpi )
  target_link_libraries(compute simgrid smpi )
  target_link_libraries(compute2 simgrid smpi )
  target_link_libraries(compute3 simgrid smpi )
  target_link_libraries(pingpong simgrid smpi )
  target_link_libraries(scatter simgrid smpi )
  target_link_libraries(reduce simgrid smpi )
  target_link_libraries(split simgrid smpi )
  target_link_libraries(smpi_sendrecv simgrid smpi )
  target_link_libraries(ttest01 simgrid smpi )
  target_link_libraries(vector_test simgrid smpi )
  target_link_libraries(hvector_test simgrid smpi )
  target_link_libraries(indexed_test simgrid smpi )
  target_link_libraries(struct_test simgrid smpi )

  set_target_properties(smpi_sendrecv PROPERTIES RENAME sendrecv)

  if(NOT WIN32)
    add_executable(dsend dsend.c)
    target_link_libraries(dsend m simgrid smpi )
  endif()

endif()


set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/bcast.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/compute.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/hvector.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/indexed.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/pt2pt.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/reduce.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/struct.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/vector.tesh  
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/alltoallv.c
  ${CMAKE_CURRENT_SOURCE_DIR}/get_processor_name.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pingpong.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bcast.c
  ${CMAKE_CURRENT_SOURCE_DIR}/allreduce.c
  ${CMAKE_CURRENT_SOURCE_DIR}/alltoall_basic.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv.c
  ${CMAKE_CURRENT_SOURCE_DIR}/reduce.c
  ${CMAKE_CURRENT_SOURCE_DIR}/compute2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/split.c
  ${CMAKE_CURRENT_SOURCE_DIR}/dsend.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ttest01.c
  ${CMAKE_CURRENT_SOURCE_DIR}/compute.c
  ${CMAKE_CURRENT_SOURCE_DIR}/compute3.c
  ${CMAKE_CURRENT_SOURCE_DIR}/alltoall2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/scatter.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sendtest.c
  ${CMAKE_CURRENT_SOURCE_DIR}/barrier.c
  ${CMAKE_CURRENT_SOURCE_DIR}/vector_test.c
  ${CMAKE_CURRENT_SOURCE_DIR}/hvector_test.c
  ${CMAKE_CURRENT_SOURCE_DIR}/indexed_test.c
  ${CMAKE_CURRENT_SOURCE_DIR}/struct_test.c
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/hostfile
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  
  PARENT_SCOPE
  )
