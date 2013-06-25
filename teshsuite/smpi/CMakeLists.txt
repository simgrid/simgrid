cmake_minimum_required(VERSION 2.6)

if(enable_smpi)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  endif()
  
  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
  
  include_directories("${CMAKE_HOME_DIRECTORY}/include/smpi")
  add_executable(gather_coll gather_coll.c)
  add_executable(allgather_coll allgather_coll.c)
  add_executable(allgatherv_coll allgatherv_coll.c)
  add_executable(alltoall2 alltoall2.c)
  add_executable(alltoall_basic alltoall_basic.c)
  add_executable(alltoall_coll alltoall_coll.c)
  add_executable(alltoallv_coll alltoallv_coll.c)
  add_executable(allreduce allreduce.c)
  add_executable(allreduce_coll allreduce_coll.c)  
  add_executable(bcast bcast.c)
  add_executable(bcast_coll bcast_coll.c) 
  add_executable(barrier_coll barrier.c )
  add_executable(compute compute.c)
  add_executable(compute2 compute2.c)
  add_executable(compute3 compute3.c)
  add_executable(shared shared.c)
  add_executable(pingpong pingpong.c)
  add_executable(scatter scatter.c)
  add_executable(reduce reduce.c)
  add_executable(reduce_coll reduce_coll.c)
  add_executable(reduce_scatter_coll reduce_scatter_coll.c)
  add_executable(split split.c)
  add_executable(smpi_sendrecv sendrecv.c)
  add_executable(ttest01 ttest01.c)
  add_executable(vector_test vector_test.c)
  add_executable(hvector_test hvector_test.c)
  add_executable(indexed_test indexed_test.c)
  add_executable(struct_test struct_test.c)

  target_link_libraries(gather_coll simgrid)
  target_link_libraries(allgather_coll simgrid)
  target_link_libraries(allgatherv_coll simgrid)
  target_link_libraries(alltoall2 simgrid)
  target_link_libraries(alltoall_basic simgrid)
  target_link_libraries(alltoall_coll simgrid)
  target_link_libraries(alltoallv_coll simgrid)
  target_link_libraries(allreduce simgrid)
  target_link_libraries(allreduce_coll simgrid)      
  target_link_libraries(bcast simgrid)
  target_link_libraries(bcast_coll simgrid)      
  target_link_libraries(barrier_coll simgrid)
  target_link_libraries(compute simgrid)
  target_link_libraries(shared simgrid)
  target_link_libraries(compute2 simgrid)
  target_link_libraries(compute3 simgrid)
  target_link_libraries(pingpong simgrid)
  target_link_libraries(scatter simgrid)
  target_link_libraries(reduce simgrid)
  target_link_libraries(reduce_coll simgrid)      
  target_link_libraries(reduce_scatter_coll simgrid)      
  target_link_libraries(split simgrid)
  target_link_libraries(smpi_sendrecv simgrid)
  target_link_libraries(ttest01 simgrid)
  target_link_libraries(vector_test simgrid)
  target_link_libraries(hvector_test simgrid)
  target_link_libraries(indexed_test simgrid)
  target_link_libraries(struct_test simgrid)

  set_target_properties(smpi_sendrecv PROPERTIES RENAME sendrecv)

  if(NOT WIN32)
    add_executable(dsend dsend.c)
    target_link_libraries(dsend m simgrid)
  endif()

endif()


set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/gather_coll.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/allgather_coll.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/allgatherv_coll.tesh  
  ${CMAKE_CURRENT_SOURCE_DIR}/allreduce_coll.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/allreduce_coll_large.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/alltoall_coll.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/alltoallv_coll.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/scatter_coll.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/barrier_coll.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/bcast.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/bcast_coll.tesh  
  ${CMAKE_CURRENT_SOURCE_DIR}/compute.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/shared.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/hvector.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/indexed.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/pt2pt.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/reduce.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/reduce_coll.tesh  
  ${CMAKE_CURRENT_SOURCE_DIR}/reduce_scatter_coll.tesh  
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
  ${CMAKE_CURRENT_SOURCE_DIR}/gather_coll.c
  ${CMAKE_CURRENT_SOURCE_DIR}/allgather_coll.c
  ${CMAKE_CURRENT_SOURCE_DIR}/allgatherv_coll.c  
  ${CMAKE_CURRENT_SOURCE_DIR}/allreduce_coll.c
  ${CMAKE_CURRENT_SOURCE_DIR}/alltoall_coll.c
  ${CMAKE_CURRENT_SOURCE_DIR}/alltoallv_coll.c  
  ${CMAKE_CURRENT_SOURCE_DIR}/bcast_coll.c
  ${CMAKE_CURRENT_SOURCE_DIR}/reduce_coll.c
  ${CMAKE_CURRENT_SOURCE_DIR}/reduce_scatter_coll.c
  ${CMAKE_CURRENT_SOURCE_DIR}/alltoallv_coll.c
  ${CMAKE_CURRENT_SOURCE_DIR}/get_processor_name.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pingpong.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bcast.c
  ${CMAKE_CURRENT_SOURCE_DIR}/allreduce.c
  ${CMAKE_CURRENT_SOURCE_DIR}/alltoall_basic.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv.c
  ${CMAKE_CURRENT_SOURCE_DIR}/reduce.c
  ${CMAKE_CURRENT_SOURCE_DIR}/compute2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/shared.c
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
