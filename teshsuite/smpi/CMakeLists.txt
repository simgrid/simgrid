if(enable_smpi)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  foreach(x coll-allgather coll-allgatherv coll-allreduce coll-allreduce-with-leaks coll-alltoall coll-alltoallv coll-barrier coll-bcast
            coll-gather coll-reduce coll-reduce-scatter coll-scatter macro-sample pt2pt-dsend pt2pt-pingpong
            type-hvector type-indexed type-struct type-vector bug-17132 gh-139 timers privatization
            io-simple io-simple-at io-all io-all-at io-shared io-ordered topo-cart-sub replay-ti-colls)
    add_executable       (${x}  EXCLUDE_FROM_ALL ${x}/${x}.c)
    target_link_libraries(${x}  simgrid)
    set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
    add_dependencies(tests ${x})
  endforeach()

  if(NOT WIN32)
    foreach(x macro-shared auto-shared macro-partial-shared macro-partial-shared-communication )
      add_executable       (${x}  EXCLUDE_FROM_ALL ${x}/${x}.c)
      target_link_libraries(${x}  simgrid)
      set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
      add_dependencies(tests ${x})
    endforeach()
  endif()

  if(enable_smpi AND SMPI_FORTRAN)
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpif90")
    add_executable       (fort_args EXCLUDE_FROM_ALL fort_args/fort_args.f90)
    target_link_libraries(fort_args simgrid)
    target_link_libraries(fort_args mpi)
    set_target_properties(fort_args PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/fort_args)
    add_dependencies(tests fort_args)
  endif()
endif()

foreach(x coll-allgather coll-allgatherv coll-allreduce coll-allreduce-with-leaks coll-alltoall coll-alltoallv coll-barrier coll-bcast
    coll-gather coll-reduce coll-reduce-scatter coll-scatter macro-sample pt2pt-dsend pt2pt-pingpong
    type-hvector type-indexed type-struct type-vector bug-17132 gh-139 timers privatization
    macro-shared auto-shared macro-partial-shared macro-partial-shared-communication
    io-simple io-simple-at io-all io-all-at io-shared io-ordered topo-cart-sub replay-ti-colls)
  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)
endforeach()

set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/fort_args/fort_args.f90 PARENT_SCOPE)
set(tesh_files    ${tesh_files}     ${CMAKE_CURRENT_SOURCE_DIR}/coll-allreduce/coll-allreduce-large.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/coll-allreduce/coll-allreduce-automatic.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/coll-allreduce/coll-allreduce-papi.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/coll-allreduce-with-leaks/mc-coll-allreduce-with-leaks.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/coll-alltoall/clusters.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/coll-alltoall/griffon.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/pt2pt-pingpong/broken_hostfiles.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/pt2pt-pingpong/TI_output.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/fort_args/fort_args.tesh  PARENT_SCOPE)
set(bin_files       ${bin_files}    ${CMAKE_CURRENT_SOURCE_DIR}/hostfile
                                    ${CMAKE_CURRENT_SOURCE_DIR}/hostfile_cluster
                                    ${CMAKE_CURRENT_SOURCE_DIR}/hostfile_griffon
                                    ${CMAKE_CURRENT_SOURCE_DIR}/hostfile_coll
                                    ${CMAKE_CURRENT_SOURCE_DIR}/hostfile_mpich
                                    ${CMAKE_CURRENT_SOURCE_DIR}/hostfile_io
                                    ${CMAKE_CURRENT_SOURCE_DIR}/hostfile_empty  PARENT_SCOPE)


if(enable_smpi)
  if(NOT WIN32)
    ADD_TESH_FACTORIES(tesh-smpi-macro-shared "*" --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/macro-shared --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/macro-shared ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/macro-shared/macro-shared.tesh)
    ADD_TESH_FACTORIES(tesh-smpi-auto-shared "*" --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/auto-shared --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/auto-shared ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/auto-shared/auto-shared.tesh)
    ADD_TESH_FACTORIES(tesh-smpi-macro-partial-shared "*" --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/macro-partial-shared --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/macro-partial-shared ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/macro-partial-shared/macro-partial-shared.tesh)
    ADD_TESH_FACTORIES(tesh-smpi-macro-partial-shared-communication "*" --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/macro-partial-shared-communication --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/macro-partial-shared-communication ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/macro-partial-shared-communication/macro-partial-shared-communication.tesh)
  endif()

  foreach(x coll-allgather coll-allgatherv coll-allreduce coll-alltoall coll-alltoallv coll-barrier coll-bcast
            coll-gather coll-reduce coll-reduce-scatter coll-scatter macro-sample pt2pt-dsend pt2pt-pingpong
    type-hvector type-indexed type-struct type-vector bug-17132 timers io-simple io-simple-at io-all io-all-at io-shared io-ordered topo-cart-sub)
    ADD_TESH_FACTORIES(tesh-smpi-${x} "*" --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms  --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/${x} --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/${x} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/${x}/${x}.tesh)
  endforeach()

  if(SMPI_FORTRAN)
    ADD_TESH_FACTORIES(tesh-smpi-fort_args "*" --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/fort_args --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/fort_args ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/fort_args/fort_args.tesh)
  endif()

  foreach (ALLGATHER 2dmesh 3dmesh bruck GB loosely_lr NTSLR NTSLR_NB pair rdb  rhv ring SMP_NTS smp_simple spreading_simple
                     ompi mpich ompi_neighborexchange mvapich2 mvapich2_smp impi)
    ADD_TESH(tesh-smpi-coll-allgather-${ALLGATHER} --cfg smpi/allgather:${ALLGATHER} --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-allgather --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-allgather ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-allgather/coll-allgather.tesh)
  endforeach()

  foreach (ALLGATHERV GB pair ring ompi mpich ompi_neighborexchange ompi_bruck mpich_rdb mpich_ring mvapich2 impi)
    ADD_TESH(tesh-smpi-coll-allgatherv-${ALLGATHERV} --cfg smpi/allgatherv:${ALLGATHERV} --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-allgatherv --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-allgatherv ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-allgatherv/coll-allgatherv.tesh)
  endforeach()

  foreach (ALLREDUCE lr rab1 rab2 rab_rdb rdb smp_binomial smp_binomial_pipeline smp_rdb smp_rsag smp_rsag_lr impi
                     smp_rsag_rab redbcast ompi mpich ompi_ring_segmented mvapich2 mvapich2_rs mvapich2_two_level)
    ADD_TESH(tesh-smpi-coll-allreduce-${ALLREDUCE} --cfg smpi/allreduce:${ALLREDUCE} --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-allreduce --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-allreduce ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-allreduce/coll-allreduce.tesh)
  endforeach()

  foreach (ALLTOALL 2dmesh 3dmesh pair pair_rma pair_one_barrier pair_light_barrier pair_mpi_barrier rdb ring
                    ring_light_barrier ring_mpi_barrier ring_one_barrier bruck basic_linear ompi mpich mvapich2
                    mvapich2_scatter_dest impi)
    ADD_TESH(tesh-smpi-coll-alltoall-${ALLTOALL} --cfg smpi/alltoall:${ALLTOALL} --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-alltoall --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-alltoall ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-alltoall/coll-alltoall.tesh)
  endforeach()

  foreach (ALLTOALLV pair pair_light_barrier pair_mpi_barrier pair_one_barrier  ring ring_light_barrier ring_mpi_barrier
                     ring_one_barrier bruck ompi mpich mvapich2 ompi_basic_linear impi)
    ADD_TESH(tesh-smpi-coll-alltoallv-${ALLTOALLV} --cfg smpi/alltoallv:${ALLTOALLV} --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-alltoallv --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-alltoallv ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-alltoallv/coll-alltoallv.tesh)
  endforeach()

  foreach (BARRIER ompi mpich mpich_smp ompi_basic_linear ompi_tree ompi_bruck ompi_recursivedoubling ompi_doublering mvapich2_pair mvapich2 impi)
      ADD_TESH(tesh-smpi-coll-barrier-${BARRIER} --cfg smpi/barrier:${BARRIER} --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-barrier --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-barrier ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-barrier/coll-barrier.tesh)
  endforeach()

  foreach (BCAST arrival_pattern_aware arrival_pattern_aware_wait arrival_scatter binomial_tree flattree
                 flattree_pipeline NTSB NTSL NTSL_Isend scatter_LR_allgather scatter_rdb_allgather SMP_binary
                 SMP_binomial SMP_linear ompi mpich ompi_split_bintree ompi_pipeline mvapich2 mvapich2_intra_node
                 mvapich2_knomial_intra_node impi)
    ADD_TESH(tesh-smpi-coll-bcast-${BCAST} --cfg smpi/bcast:${BCAST} --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-bcast --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-bcast ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-bcast/coll-bcast.tesh)
  endforeach()

  foreach (GATHER ompi mpich ompi_basic_linear ompi_linear_sync ompi_binomial mvapich2 mvapich2_two_level impi)
    ADD_TESH(tesh-smpi-coll-gather-${GATHER} --cfg smpi/gather:${GATHER} --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-gather --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-gather ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-gather/coll-gather.tesh)
  endforeach()

  foreach (REDUCE arrival_pattern_aware binomial flat_tree NTSL scatter_gather ompi mpich ompi_chain ompi_binary impi
                  ompi_basic_linear ompi_binomial ompi_in_order_binary mvapich2 mvapich2_knomial mvapich2_two_level rab)
    ADD_TESH(tesh-smpi-coll-reduce-${REDUCE} --cfg smpi/reduce:${REDUCE} --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-reduce --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-reduce ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-reduce/coll-reduce.tesh)
  endforeach()

  foreach (REDUCE_SCATTER ompi mpich ompi_basic_recursivehalving ompi_ring mpich_noncomm mpich_pair mvapich2 mpich_rdb impi)
    ADD_TESH(tesh-smpi-coll-reduce-scatter-${REDUCE_SCATTER} --cfg smpi/reduce_scatter:${REDUCE_SCATTER} --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-reduce-scatter --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-reduce-scatter ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-reduce-scatter/coll-reduce-scatter.tesh)
  endforeach()

  foreach (SCATTER ompi mpich ompi_basic_linear ompi_binomial mvapich2 mvapich2_two_level_binomial mvapich2_two_level_direct impi)
    ADD_TESH(tesh-smpi-coll-scatter-${SCATTER} --cfg smpi/scatter:${SCATTER} --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-scatter --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-scatter ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-scatter/coll-scatter.tesh)
  endforeach()

  # Extra allreduce test: large automatic
  ADD_TESH(tesh-smpi-coll-allreduce-large --cfg smpi/allreduce:ompi_ring_segmented --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-allreduce --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-allreduce ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-allreduce/coll-allreduce-large.tesh)
  ADD_TESH(tesh-smpi-coll-allreduce-automatic --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-allreduce --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-allreduce ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-allreduce/coll-allreduce-automatic.tesh)

  # Extra alltoall test: cluster-types
  ADD_TESH(tesh-smpi-cluster-types --cfg smpi/alltoall:mvapich2 --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-alltoall --setenv libdir=${CMAKE_BINARY_DIR}/lib --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-alltoall ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-alltoall/clusters.tesh)

  # Extra allreduce test : PAPI tracing
  if (HAVE_PAPI)
    ADD_TESH(tesh-smpi-papi-tracing --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-allreduce --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-allreduce ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-allreduce/coll-allreduce-papi.tesh)
  endif()

  # Extra allreduce test : with leak : don't run on asan (and ignore leaks on valgrind)
  if (NOT HAVE_SANITIZER_ADDRESS)
    ADD_TESH(tesh-smpi-coll-allreduce-with-leaks --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-allreduce-with-leaks --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-allreduce-with-leaks ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-allreduce-with-leaks/coll-allreduce-with-leaks.tesh)
    set_target_properties(coll-allreduce-with-leaks PROPERTIES COMPILE_FLAGS "-trace-call-location")
    if(enable_model-checking)
      add_dependencies(tests-mc coll-allreduce-with-leaks)
      ADD_TESH(tesh-mc-smpi-coll-allreduce-with-leaks --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-allreduce-with-leaks --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/coll-allreduce-with-leaks ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/coll-allreduce-with-leaks/mc-coll-allreduce-with-leaks.tesh)
    endif()
  endif()

  # Extra pt2pt pingpong test: broken usage ti-tracing
  ADD_TESH_FACTORIES(tesh-smpi-broken  "thread"   --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/pt2pt-pingpong --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/pt2pt-pingpong ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/pt2pt-pingpong/broken_hostfiles.tesh)
  ADD_TESH(tesh-smpi-replay-ti-tracing            --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/pt2pt-pingpong --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/pt2pt-pingpong ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/pt2pt-pingpong/TI_output.tesh)
  ADD_TESH(tesh-smpi-replay-ti-tracing-coll       --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/replay-ti-colls --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/replay-ti-colls ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/replay-ti-colls/replay-ti-colls.tesh)

  ADD_TESH_FACTORIES(tesh-smpi-gh-139  "thread"   --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/gh-139 --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/gh-139 ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/gh-139/gh-139.tesh)

  # Simple privatization tests
  if(HAVE_PRIVATIZATION)
    foreach(PRIVATIZATION dlopen mmap)
      ADD_TESH_FACTORIES(tesh-smpi-privatization-${PRIVATIZATION}  "*" --setenv privatization=${PRIVATIZATION} --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/privatization --cd ${CMAKE_BINARY_DIR}/teshsuite/smpi/privatization ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/privatization/privatization.tesh)
    endforeach()
  endif()
endif()
