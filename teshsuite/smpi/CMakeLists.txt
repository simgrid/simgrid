if(enable_smpi)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  foreach(x coll-allgather coll-allgatherv coll-allreduce coll-alltoall coll-alltoallv coll-barrier coll-bcast 
            coll-gather coll-reduce coll-reduce-scatter coll-scatter macro-sample pt2pt-dsend pt2pt-pingpong 
            type-hvector type-indexed type-struct type-vector bug-17132)
    add_executable       (${x}  ${x}/${x}.c)
    target_link_libraries(${x}  simgrid)
    set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})

    set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
    set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)

     ADD_TESH_FACTORIES(tesh-smpi-${x} "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/${x} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/${x} ${x}.tesh)
  endforeach()

  if(NOT WIN32)
    add_executable       (macro-shared macro-shared/macro-shared.c)
    target_link_libraries(macro-shared simgrid)
    set_target_properties(macro-shared PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/macro-shared)

    ADD_TESH_FACTORIES(tesh-smpi-macro-shared "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/macro-shared --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/macro-shared macro-shared.tesh)
  endif()

  foreach (ALLGATHER 2dmesh 3dmesh bruck GB loosely_lr NTSLR_NB pair rdb  rhv ring SMP_NTS smp_simple spreading_simple 
                     ompi mpich ompi_neighborexchange mvapich2 mvapich2_smp impi)
    ADD_TESH(tesh-smpi-coll-allgather-${ALLGATHER} --cfg smpi/allgather:${ALLGATHER} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-allgather --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-allgather coll-allgather.tesh)
  endforeach()

  foreach (ALLGATHERV GB pair ring ompi mpich ompi_neighborexchange ompi_bruck mpich_rdb mpich_ring mvapich2 impi)
    ADD_TESH(tesh-smpi-coll-allgatherv-${ALLGATHERV} --cfg smpi/allgatherv:${ALLGATHERV} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-allgatherv --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-allgatherv coll-allgatherv.tesh)
  endforeach()

  foreach (ALLREDUCE lr rab1 rab2 rab_rdb rdb smp_binomial smp_binomial_pipeline smp_rdb smp_rsag smp_rsag_lr impi
                     smp_rsag_rab redbcast ompi mpich ompi_ring_segmented mvapich2 mvapich2_rs mvapich2_two_level)
    ADD_TESH(tesh-smpi-coll-allreduce-${ALLREDUCE} --cfg smpi/allreduce:${ALLREDUCE} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-allreduce --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-allreduce coll-allreduce.tesh)
  endforeach()

  foreach (ALLTOALL 2dmesh 3dmesh pair pair_rma pair_one_barrier pair_light_barrier pair_mpi_barrier rdb ring 
                    ring_light_barrier ring_mpi_barrier ring_one_barrier bruck basic_linear ompi mpich mvapich2 
                    mvapich2_scatter_dest impi)
    ADD_TESH(tesh-smpi-coll-alltoall-${ALLTOALL} --cfg smpi/alltoall:${ALLTOALL} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-alltoall --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-alltoall coll-alltoall.tesh)
  endforeach()

  foreach (ALLTOALLV pair pair_light_barrier pair_mpi_barrier pair_one_barrier  ring ring_light_barrier ring_mpi_barrier
                     ring_one_barrier bruck ompi mpich mvapich2 ompi_basic_linear impi)
    ADD_TESH(tesh-smpi-coll-alltoallv-${ALLTOALLV} --cfg smpi/alltoallv:${ALLTOALLV} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-alltoallv --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-alltoallv coll-alltoallv.tesh)
  endforeach()

  foreach (BARRIER ompi mpich ompi_basic_linear ompi_tree ompi_bruck ompi_recursivedoubling ompi_doublering mvapich2_pair mvapich2 impi)
      ADD_TESH(tesh-smpi-coll-barrier-${BARRIER} --cfg smpi/barrier:${BARRIER} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-barrier --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-barrier coll-barrier.tesh)
  endforeach()

  foreach (BCAST arrival_pattern_aware arrival_pattern_aware_wait arrival_scatter binomial_tree flattree 
                 flattree_pipeline NTSB NTSL NTSL_Isend scatter_LR_allgather scatter_rdb_allgather SMP_binary 
                 SMP_binomial SMP_linear ompi mpich ompi_split_bintree ompi_pipeline mvapich2 mvapich2_intra_node 
                 mvapich2_knomial_intra_node impi)
    ADD_TESH(tesh-smpi-coll-bcast-${BCAST} --cfg smpi/bcast:${BCAST} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-bcast --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-bcast coll-bcast.tesh)
  endforeach()

  foreach (GATHER ompi mpich ompi_basic_linear ompi_linear_sync ompi_binomial mvapich2 mvapich2_two_level impi)
    ADD_TESH(tesh-smpi-coll-gather-${GATHER} --cfg smpi/gather:${GATHER} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-gather --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-gather coll-gather.tesh)
  endforeach()

  foreach (REDUCE arrival_pattern_aware binomial flat_tree NTSL scatter_gather ompi mpich ompi_chain ompi_binary impi
                  ompi_basic_linear ompi_binomial ompi_in_order_binary mvapich2 mvapich2_knomial mvapich2_two_level rab)
    ADD_TESH(tesh-smpi-coll-reduce-${REDUCE} --cfg smpi/reduce:${REDUCE} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-reduce --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-reduce coll-reduce.tesh)
  endforeach()

  foreach (REDUCE_SCATTER ompi mpich ompi_basic_recursivehalving ompi_ring mpich_noncomm mpich_pair mvapich2 mpich_rdb impi)
    ADD_TESH(tesh-smpi-coll-reduce-scatter-${REDUCE_SCATTER} --cfg smpi/reduce_scatter:${REDUCE_SCATTER} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-reduce-scatter --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-reduce-scatter coll-reduce-scatter.tesh)
  endforeach()

  foreach (SCATTER ompi mpich ompi_basic_linear ompi_binomial mvapich2 mvapich2_two_level_binomial mvapich2_two_level_direct impi)
    ADD_TESH(tesh-smpi-coll-scatter-${SCATTER} --cfg smpi/scatter:${SCATTER} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-scatter --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-scatter coll-scatter.tesh)
  endforeach()

  # Extra allreduce test: large automatic
  ADD_TESH(tesh-smpi-coll-allreduce-large --cfg smpi/allreduce:ompi_ring_segmented --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-allreduce --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-allreduce coll-allreduce-large.tesh)
  ADD_TESH(tesh-smpi-coll-allreduce-automatic --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-allreduce --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-allreduce coll-allreduce-automatic.tesh)

  # Extra allreduce test: cluster-types
  ADD_TESH(tesh-smpi-cluster-types --cfg smpi/alltoall:mvapich2 --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-alltoall --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-alltoall clusters.tesh)

  # Extra pt2pt pingpong test: broken usage ti-tracing
  ADD_TESH_FACTORIES(tesh-smpi-broken  "thread"   --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/pt2pt-pingpong --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/pt2pt-pingpong broken_hostfiles.tesh)
  ADD_TESH(tesh-smpi-replay-ti-tracing            --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/pt2pt-pingpong --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/pt2pt-pingpong TI_output.tesh)
endif()

set(teshsuite_src ${teshsuite_src}  ${CMAKE_CURRENT_SOURCE_DIR}/macro-shared/macro-shared.c                PARENT_SCOPE)
set(tesh_files    ${tesh_files}     ${CMAKE_CURRENT_SOURCE_DIR}/coll-allreduce/coll-allreduce-large.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/coll-allreduce/coll-allreduce-automatic.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/coll-alltoall/clusters.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/macro-shared/macro-shared.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/pt2pt-pingpong/broken_hostfiles.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/pt2pt-pingpong/TI_output.tesh              PARENT_SCOPE)
set(bin_files       ${bin_files}    ${CMAKE_CURRENT_SOURCE_DIR}/hostfile
                                    ${CMAKE_CURRENT_SOURCE_DIR}/hostfile_cluster
                                    ${CMAKE_CURRENT_SOURCE_DIR}/hostfile_coll
                                    ${CMAKE_CURRENT_SOURCE_DIR}/hostfile_empty                             PARENT_SCOPE)
