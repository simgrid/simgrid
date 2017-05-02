if(enable_smpi AND enable_smpi_MPICH3_testsuite)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

  foreach(file anyall bottom eagerdt huge_anysrc huge_underflow inactivereq isendself isendirecv isendselfprobe issendselfcancel cancelanysrc pingping probenull
          dtype_send probe-unexp sendall sendflood sendrecv1 sendrecv2 sendrecv3 waitany-null waittestnull many_isend manylmt recv_any)
    # not compiled files: big_count_status bsend1 bsend2 bsend3 bsend4 bsend5 bsendalign bsendfrag bsendpending mprobe
    # cancelrecv greq1 icsend large_message pscancel rcancel rqfreeb rqstatus scancel2 scancel sendself scancel_unmatch
    add_executable(${file} ${file}.c)
    target_link_libraries(${file} simgrid mtest_c)
  endforeach()
endif()

if (enable_smpi_MPICH3_testsuite AND HAVE_RAW_CONTEXTS)
  ADD_TEST(test-smpi-mpich3-pt2pt-raw      ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/pt2pt ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests ${TESH_OPTION} -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/pt2pt -tests=testlist -execarg=--cfg=contexts/factory:raw -execarg=--cfg=smpi/privatization:${HAVE_PRIVATIZATION})
  SET_TESTS_PROPERTIES(test-smpi-mpich3-pt2pt-raw PROPERTIES PASS_REGULAR_EXPRESSION "tests passed!")
endif()

foreach(file anyall bottom eagerdt huge_anysrc huge_underflow inactivereq isendself isendirecv isendselfprobe issendselfcancel pingping probenull
          probe-unexp sendall sendflood sendrecv1 sendrecv2 sendrecv3 waitany-null waittestnull
          big_count_status bsend1 bsend2 bsend3 bsend4 bsend5 bsendalign bsendfrag bsendpending
          cancelrecv cancelanysrc dtype_send greq1 icsend large_message pscancel rcancel rqfreeb rqstatus scancel2 scancel sendself many_isend manylmt mprobe recv_any scancel_unmatch
)
  set(examples_src  ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${file}.c)
endforeach()

set(examples_src  ${examples_src}                                        PARENT_SCOPE)
set(txt_files     ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/testlist  PARENT_SCOPE)
