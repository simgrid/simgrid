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
          dtype_send greq1 probe-unexp rqstatus sendall sendflood sendrecv1 sendrecv2 sendrecv3 waitany-null waittestnull many_isend manylmt recv_any sendself scancel scancel2 rcancel bsend1 bsend2 bsend3 bsend4 bsend5 bsendalign bsendfrag bsendpending rqfreeb)
    # not compiled files: big_count_status mprobe
    # cancelrecv  icsend large_message pscancel     scancel_unmatch
    add_executable(${file} EXCLUDE_FROM_ALL ${file}.c)
    add_dependencies(tests ${file})
    target_link_libraries(${file} simgrid mtest_c)
  endforeach()

  if(HAVE_RAW_CONTEXTS AND (NOT enable_memcheck) AND (NOT enable_address_sanitizer) AND (NOT enable_undefined_sanitizer) AND (NOT enable_thread_sanitizer))
    set(facto "--cfg=contexts/factory:raw")
    set(name raw)
  else()
    set(facto "--cfg=contexts/factory:thread")
    set(name thread)
  endif()
  ADD_TEST(test-smpi-mpich3-pt2pt-${name}      ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/pt2pt ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests "-wrapper=${VALGRIND_WRAPPER}" -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/pt2pt -tests=testlist -execarg=${facto} )
  SET_TESTS_PROPERTIES(test-smpi-mpich3-pt2pt-${name} PROPERTIES PASS_REGULAR_EXPRESSION "tests passed!")
  unset(facto)
  unset(name)
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
