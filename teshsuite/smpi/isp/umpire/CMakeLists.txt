# See http://formalverification.cs.utah.edu/ISP_Tests/

set(umpire_tests_passing
  change-send-buffer
  comm-dup-no-error
  comm-dup-no-free
  comm-dup-no-free2
  comm-simple
  comm-split-no-free
  comm-translate-ranks
  hello
  no-error
  no-error2
  no-error3
  no-error3-any_src
  no-error-any_src
  no-error-interleaved-isend
  no-error-test
  no-error-testall
  no-error-testany
  no-error-vector-isend
  no-error-wait
  no-error-waitany-any_src2
  no-error-waitany-any_src3
  no-error-waitany
  no-error-wait-any_src
  no-error-wait-any_src2
  no-error-wait-any_src3
  no-error-wait-any_src4
  no-error-waitany-any_src
  partial-recv
  sendrecv-deadlock
  send-recv-ok
  irecv-isend-ok
  irecv-isend-ok2
  lost-request-waitall
 ) # Passing (no deadlock) tests

# These are supposed to deadlock but ISP does not find deadlock
# and we don't either. What should be test here?
set(umpire_tests_missed_deadlock
  any_src-deadlock
  any_src-can-deadlock
  any_src-can-deadlock2
  any_src-can-deadlock3
  )

set(umpire_tests_deadlock
  any_src-can-deadlock10
  any_src-can-deadlock10_mod
  any_src-can-deadlock11
  any_src-can-deadlock4
  any_src-can-deadlock4_mod
  any_src-can-deadlock5
  any_src-can-deadlock5_mod
  any_src-can-deadlock6
  any_src-can-deadlock6_mod
  any_src-can-deadlock7
  any_src-can-deadlock8
  any_src-can-deadlock9
  any_src-waitall-deadlock
  any_src-waitall-deadlock2
  any_src-waitall-deadlock3
  any_src-waitany-deadlock2
  any_src-waitany-deadlock
  any_src-wait-deadlock
  any_src-wait-deadlock2
  basic-deadlock
  basic-deadlock-comm_create
  basic-deadlock-comm_dup
  basic-deadlock-comm_split
  bcast-deadlock
  collective-misorder
  collective-misorder2
  collective-misorder-allreduce
  comm-bcast-deadlock
  comm-deadlock
  complex-deadlock
  dropped-req
  finalize-deadlock
  irecv-deadlock
  waitall-deadlock
  waitany-deadlock
  wait-deadlock
 ) # failing (deadlocking) tests
 
set(umpire_tests_problematic
  deadlock-config # No deadlock detected by ISP unless MPI_Send blocking activated

 ) # problematic tests
 
 # These files are not listed on http://formalverification.cs.utah.edu/ISP_Tests/
set(umpire_src_other
  abort1.c
  abort2.c
  abort3.c
  abort.c
  basic-deadlock-cart_create.c
  basic-deadlock-cart_sub.c
  basic-deadlock-graph_create.c
  basic-deadlock-intercomm_create.c
  basic-deadlock-intercomm_merge.c
  change-send-buffer-exhaustive.c
  change-send-buffer-type-exhaustive.c
  collective-exhaustive-byte-int-mismatch.c
  collective-exhaustive-no-error.c
  collective-misorder.c
  collective-misorder2.c

  deadlock-config_blocking.c
  errhandler-no-error.c
  errhandler-no-free.c
  group-no-error2.c
  group-no-error3.c
  group-no-error.c
  group-no-error-exhaustive.c
  group-no-free2.c
  group-no-free3.c
  group-no-free.c
  group-no-free-exhaustive.c
  intercomm_create-deadlock2.c
  intercomm_create-deadlock3.c
  intercomm_create-deadlock4.c
  intercomm_create-deadlock.c
  intercomm_create-no-error.c
  intercomm_merge-deadlock.c
  lost-request2.c
  lost-request3.c
  lost-request.c  
  no-error4-any_src.c
  no-error4.c
  no-error-derived-comms.c
    
  no-error-persistent-all-completions.c
  no-error-persistent.c
  no-error-persistent-test.c
  no-error-persistent-testpartial.c
  no-error-persistent-waitpartial.c
  no-error-probe-any_src.c
  no-error-probe-any_tag.c
  no-error-probe.c
  
  no-error-wait2.c # Does not compile.
  no-error-waitall-any_src2.c
  no-error-waitall-any_src3.c
  no-error-waitall-any_src.c
  no-error-waitall.c
  no-error-waitany2.c
  
  op-no-error.c
  op-no-free.c
  partial-recv-exhaustive.c
  partial-recv-persistent2.c
  partial-recv-persistent3.c
  partial-recv-persistent4.c
  partial-recv-persistent.c
  persistent2.c
  persistent3.c
  persistent4.c
  persistent.c
  probe-any_src-types-can-mismatch.c
  probe-any_tag-types-can-mismatch.c
  probe-deadlock.c
  pt2pt-byte-int-mismatch.c
  remote_group-no-error.c
  type-commit-twice.c
  type-no-error.c
  type-no-error-exhaustive.c
  type-no-error-exhaustive-with-isends.c
  type-no-free2.c
  type-no-free3.c
  type-no-free.c
  type-no-free-exhaustive.c
  )

# Add these files to the archive
#################

set(sources "${CMAKE_CURRENT_SOURCE_DIR}/README")
foreach(file ${umpire_src_other})
  set(sources ${sources} ${CMAKE_CURRENT_SOURCE_DIR}/${file})
endforeach()

foreach(file ${umpire_tests_passing} ${umpire_tests_deadlock} ${umpire_tests_missed_deadlock} ${umpire_tests_problematic}  ) 
  set(sources ${sources} ${CMAKE_CURRENT_SOURCE_DIR}/${file}.c)
endforeach()
set(txt_files ${txt_files} ${sources} PARENT_SCOPE)

# Build the tests
#################

if(enable_smpi AND enable_model-checking AND enable_smpi_ISP_testsuite)
  set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  foreach (test ${umpire_tests_passing} ${umpire_tests_deadlock} ${umpire_tests_problematic} )
    add_executable(${test} ${test}.c)
    target_link_libraries(${test} simgrid)
    set_source_files_properties(${test}.c PROPERTIES COMPILE_FLAGS "-Wno-error")
    set(umpire_tesh ${umpire_tesh} ${test})
    set(files_to_clean ${files_to_clean} ${CMAKE_CURRENT_BINARY_DIR}/${test}.tesh)
  endforeach(test)
  
  # TODO, what should we do about ${umpire_tests_missed_deadlock}?

  foreach (test ${umpire_tests_passing})
    write_file(${CMAKE_CURRENT_BINARY_DIR}/${test}.tesh "! timeout 30")
    write_file(${CMAKE_CURRENT_BINARY_DIR}/${test}.tesh "! output display" APPEND)
    write_file(${CMAKE_CURRENT_BINARY_DIR}/${test}.tesh "\$ \${bindir:=.}/../../../../bin/smpirun -wrapper \"\${bindir:=.}/../../../../bin/simgrid-mc\" -hostfile ../../hostfile -platform ../../../../examples/platforms/small_platform.xml --log=xbt_cfg.thresh:warning -np 3 --cfg=smpi/running_power:1e9 --cfg=smpi/coll_selector:mpich \${bindir:=.}/${test} --log=smpi_coll.thresh:error" APPEND)
  endforeach()

  foreach (test ${umpire_tests_deadlock} ${umpire_tests_problematic} )
    write_file(${CMAKE_CURRENT_BINARY_DIR}/${test}.tesh "! timeout 30"     )
    write_file(${CMAKE_CURRENT_BINARY_DIR}/${test}.tesh "! expect return 3" APPEND)
    write_file(${CMAKE_CURRENT_BINARY_DIR}/${test}.tesh "! output display"  APPEND)
    write_file(${CMAKE_CURRENT_BINARY_DIR}/${test}.tesh "\$ \${bindir:=.}/../../../../bin/smpirun -wrapper \"\${bindir:=.}/../../../../bin/simgrid-mc\" -hostfile ../../hostfile -platform ../../../../examples/platforms/small_platform.xml --log=xbt_cfg.thresh:warning -np 3 --cfg=smpi/running_power:1e9 --cfg=smpi/coll_selector:mpich \${bindir:=.}/${test} --log=smpi_coll.thresh:error" APPEND)
  endforeach()
endif()

# Erase all tesh files on cleanup: they are generated anyway
set_directory_properties ( PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${files_to_clean}" )

if(enable_smpi AND enable_model-checking AND enable_smpi_ISP_testsuite)
  foreach (tesh ${umpire_tesh})
    ADD_TESH(mc-umpire-${tesh}
      --setenv srcdir=${CMAKE_CURRENT_SOURCE_DIR}
      --setenv bindir=${CMAKE_CURRENT_BINARY_DIR}
      --cd ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_BINARY_DIR}/${tesh}.tesh)
  endforeach()
endif()
