if(enable_smpi)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  foreach(x coll-allgather coll-bcast type-hvector type-indexed type-struct type-vector)
    add_executable       (${x}  ${x}/${x}.c)
    target_link_libraries(${x}  simgrid)
    set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})

    set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
    set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)

     ADD_TESH_FACTORIES(tesh-smpi-${x} "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/${x} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/${x} ${x}.tesh)
  endforeach()

  foreach (ALLGATHER_COLL 2dmesh 3dmesh bruck GB loosely_lr NTSLR_NB pair rdb  rhv ring SMP_NTS smp_simple 
                          spreading_simple ompi mpich ompi_neighborexchange mvapich2 mvapich2_smp impi)
    ADD_TESH(tesh-smpi-coll-allgather-${ALLGATHER_COLL} --cfg smpi/allgather:${ALLGATHER_COLL} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-allgather --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-allgather coll-allgather.tesh)
  endforeach()

  foreach (BCAST_COLL arrival_pattern_aware arrival_pattern_aware_wait arrival_scatter binomial_tree flattree 
                      flattree_pipeline NTSB NTSL NTSL_Isend scatter_LR_allgather scatter_rdb_allgather SMP_binary 
                      SMP_binomial SMP_linear ompi mpich ompi_split_bintree ompi_pipeline mvapich2 mvapich2_intra_node 
                      mvapich2_knomial_intra_node impi)
    ADD_TESH(tesh-smpi-coll-bcast-${BCAST_COLL} --cfg smpi/bcast:${BCAST_COLL} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-bcast --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-bcast coll-bcast.tesh)
  endforeach()
endif()

set(bin_files       ${bin_files}      ${CMAKE_CURRENT_SOURCE_DIR}/hostfile
                                      ${CMAKE_CURRENT_SOURCE_DIR}/hostfile_coll  PARENT_SCOPE)
set(teshsuite_src ${teshsuite_src}                                             PARENT_SCOPE)
set(tesh_files    ${tesh_files}                                                PARENT_SCOPE)
