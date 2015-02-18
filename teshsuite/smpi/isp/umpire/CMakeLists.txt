cmake_minimum_required(VERSION 2.6)

set(umpire_src
  abort1.c
  abort2.c
  abort3.c
  abort.c
  any_src-can-deadlock10.c
  any_src-can-deadlock10_mod.c
  any_src-can-deadlock11.c
  any_src-can-deadlock2.c
  any_src-can-deadlock3.c
  any_src-can-deadlock4.c
  any_src-can-deadlock4_mod.c
  any_src-can-deadlock5.c
  any_src-can-deadlock5_mod.c
  any_src-can-deadlock6.c
  any_src-can-deadlock6_mod.c
  any_src-can-deadlock7.c
  any_src-can-deadlock8.c
  any_src-can-deadlock9.c
  any_src-can-deadlock.c
  any_src-deadlock.c
  any_src-waitall-deadlock2.c
  any_src-waitall-deadlock3.c
  any_src-waitall-deadlock.c
  any_src-waitany-deadlock2.c
  any_src-waitany-deadlock.c
  any_src-wait-deadlock2.c
  any_src-wait-deadlock.c
  basic-deadlock.c
  basic-deadlock-cart_create.c
  basic-deadlock-cart_sub.c
  basic-deadlock-comm_create.c
  basic-deadlock-comm_dup.c
  basic-deadlock-comm_split.c
  basic-deadlock-graph_create.c
  basic-deadlock-intercomm_create.c
  basic-deadlock-intercomm_merge.c
  bcast-deadlock.c
  change-send-buffer.c
  change-send-buffer-exhaustive.c
  change-send-buffer-type-exhaustive.c
  collective-exhaustive-byte-int-mismatch.c
  collective-exhaustive-no-error.c
  collective-misorder2.c
  collective-misorder-allreduce.c
  collective-misorder.c
  comm-bcast-deadlock.c
  comm-deadlock.c
  comm-dup-no-error.c
  comm-dup-no-free2.c
  comm-dup-no-free.c
  comm-simple.c
  comm-split-no-free.c
  comm-translate-ranks.c
  complex-deadlock.c
  deadlock-config_blocking.c
  deadlock-config.c
  dropped-req.c
  errhandler-no-error.c
  errhandler-no-free.c
  finalize-deadlock.c
  group-no-error2.c
  group-no-error3.c
  group-no-error.c
  group-no-error-exhaustive.c
  group-no-free2.c
  group-no-free3.c
  group-no-free.c
  group-no-free-exhaustive.c
  hello.c
  intercomm_create-deadlock2.c
  intercomm_create-deadlock3.c
  intercomm_create-deadlock4.c
  intercomm_create-deadlock.c
  intercomm_create-no-error.c
  intercomm_merge-deadlock.c
  irecv-deadlock.c
  irecv-isend-ok2.c
  irecv-isend-ok.c
  lost-request2.c
  lost-request3.c
  lost-request.c
  lost-request-waitall.c
  no-error2.c
  no-error3-any_src.c
  no-error3.c
  no-error4-any_src.c
  no-error4.c
  no-error-any_src.c
  no-error.c
  no-error-derived-comms.c
  no-error-interleaved-isend.c
  no-error-persistent-all-completions.c
  no-error-persistent.c
  no-error-persistent-test.c
  no-error-persistent-testpartial.c
  no-error-persistent-waitpartial.c
  no-error-probe-any_src.c
  no-error-probe-any_tag.c
  no-error-probe.c
  no-error-testall.c
  no-error-testany.c
  no-error-test.c
  no-error-vector-isend.c
  # no-error-wait2.c # Does not compile.
  no-error-waitall-any_src2.c
  no-error-waitall-any_src3.c
  no-error-waitall-any_src.c
  no-error-waitall.c
  no-error-waitany2.c
  no-error-waitany-any_src2.c
  no-error-waitany-any_src3.c
  no-error-waitany-any_src.c
  no-error-waitany.c
  no-error-wait-any_src2.c
  no-error-wait-any_src3.c
  no-error-wait-any_src4.c
  no-error-wait-any_src.c
  no-error-wait.c
  op-no-error.c
  op-no-free.c
  partial-recv.c
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
  sendrecv-deadlock.c
  send-recv-ok.c
  type-commit-twice.c
  type-no-error.c
  type-no-error-exhaustive.c
  type-no-error-exhaustive-with-isends.c
  type-no-free2.c
  type-no-free3.c
  type-no-free.c
  type-no-free-exhaustive.c
  waitall-deadlock.c
  waitany-deadlock.c
  wait-deadlock.c
  )

set(sources "")
FOREACH(s ${umpire_src})
  set(sources ${sources} ${CMAKE_CURRENT_SOURCE_DIR}/${s})
ENDFOREACH()

set(umpire_tesh "")
foreach(tesh
    any_src-can-deadlock10
    any_src-can-deadlock4
    any_src-can-deadlock5
    any_src-can-deadlock6
    any_src-waitall-deadlock2
    any_src-waitall-deadlock3
    any_src-waitany-deadlock2
    any_src-waitany-deadlock
    any_src-wait-deadlock
    basic-deadlock-comm_create
    basic-deadlock-comm_dup
    basic-deadlock-comm_split
    basic-deadlock
    bcast-deadlock
    collective-misorder-allreduce
    collective-misorder
    complex-deadlock
    deadlock-config
    finalize-deadlock
    irecv-deadlock
    no-error2
    no-error3-any_src
    no-error3
    no-error
    )
  set(umpire_tesh ${umpire_tesh} teshsuite/smpi/isp/umpire/${tesh}.tesh)
ENDFOREACH()

set(examples_src ${examples_src} ${sources} PARENT_SCOPE)
set(txt_files ${txt_files} ${CMAKE_CURRENT_SOURCE_DIR}/README PARENT_SCOPE)
set(EXTRA_DIST ${EXTRA_DIST} ${CMAKE_CURRENT_SOURCE_DIR}/no-error-wait2.c PARENT_SCOPE)
set(tesh_files ${tesh_files} ${umpire_tesh} PARENT_SCOPE)

if(enable_smpi AND enable_smpi_ISP_testsuite)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  FOREACH (s ${umpire_src})
    STRING(REGEX REPLACE "\\.c$" "" exe ${s})
    add_executable(${exe} ${s})
    target_link_libraries(${exe} simgrid)
    set_source_files_properties(${s} PROPERTIES COMPILE_FLAGS "-Wno-error")
  ENDFOREACH(s)
endif()
