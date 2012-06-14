cmake_minimum_required(VERSION 2.6)

if(enable_smpi)
set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/bin/smpicc")

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
set(flags "-Dmain=smpi_simulated_main")
set(INCLUDES_bis "-I${CMAKE_HOME_DIRECTORY}/include/smpi")

add_definitions (${INCLUDES_bis} ${flags})

add_executable(alltoall2 alltoall2.c )
add_executable(alltoall_basic alltoall_basic.c)
add_executable(alltoallv alltoallv.c)
add_executable(allreduce allreduce.c)
add_executable(bcast bcast.c)
add_executable(bcbench bcbench.c)
add_executable(compute compute.c)
add_executable(compute2 compute2.c)
add_executable(compute3 compute3.c)
add_executable(pingpong pingpong.c)
add_executable(scatter scatter.c)
add_executable(reduce reduce.c)
add_executable(split split.c)
add_executable(dsend dsend.c)
add_executable(mvmul mvmul.c)
add_executable(smpi_sendrecv sendrecv.c)
add_executable(smpi_traced tracing/smpi_traced.c)
add_executable(smpi_traced_simple tracing/smpi_traced_simple.c)
add_executable(ttest01 ttest01.c)
add_executable(mc_bugged1 mc_bugged1.c)
add_executable(mc_bugged2 mc_bugged2.c)

target_link_libraries(alltoall2 m simgrid smpi )
target_link_libraries(alltoall_basic m simgrid smpi )
target_link_libraries(alltoallv m simgrid smpi )
target_link_libraries(allreduce m simgrid smpi )
target_link_libraries(bcast m simgrid smpi )
target_link_libraries(bcbench m simgrid smpi )
target_link_libraries(compute m simgrid smpi )
target_link_libraries(compute2 m simgrid smpi )
target_link_libraries(compute3 m simgrid smpi )
target_link_libraries(pingpong m simgrid smpi )
target_link_libraries(scatter m simgrid smpi )
target_link_libraries(reduce m simgrid smpi )
target_link_libraries(split m simgrid smpi )
target_link_libraries(dsend m simgrid smpi )
target_link_libraries(mvmul m simgrid smpi )
target_link_libraries(smpi_sendrecv m simgrid smpi )
target_link_libraries(smpi_traced m simgrid smpi )
target_link_libraries(smpi_traced_simple m simgrid smpi )
target_link_libraries(ttest01 m simgrid smpi )
target_link_libraries(mc_bugged1 m simgrid smpi )
target_link_libraries(mc_bugged2 m simgrid smpi )

set_target_properties(smpi_sendrecv PROPERTIES RENAME sendrecv)
endif(enable_smpi)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/bcast.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/reduce.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/tracing/smpi_traced.tesh
  PARENT_SCOPE
)
set(xml_files
  ${xml_files}
  PARENT_SCOPE
)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/alltoallv.c
  ${CMAKE_CURRENT_SOURCE_DIR}/mc_bugged2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/get_processor_name.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pingpong.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bcast.c
  ${CMAKE_CURRENT_SOURCE_DIR}/allreduce.c
  ${CMAKE_CURRENT_SOURCE_DIR}/alltoall_basic.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv.c
  ${CMAKE_CURRENT_SOURCE_DIR}/mc_bugged1.c
  ${CMAKE_CURRENT_SOURCE_DIR}/reduce.c
  ${CMAKE_CURRENT_SOURCE_DIR}/mvmul.c
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
  ${CMAKE_CURRENT_SOURCE_DIR}/bcbench.c
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
  PARENT_SCOPE
)
