if(enable_smpi AND enable_testsuite_smpi_MPICH3)
  set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

  foreach(file cmfree cmfree2 cmsplit2 cmsplit cmsplit_type commcreate1 comm_create_group comm_group_half comm_group_rand
          comm_info comm_info2 ctxalloc ctxsplit dup dup_with_info commname)
    # not compiled files
    # comm_idup comm_idup_mul comm_idup_overlap  dupic ic1 ic2 iccreate icgroup icm icsplit probe-intercomm
    add_executable(${file} EXCLUDE_FROM_ALL ${file}.c)
    add_dependencies(tests ${file})
    target_link_libraries(${file} simgrid mtest_c)
  endforeach()
endif()

if (enable_testsuite_smpi_MPICH3 AND HAVE_RAW_CONTEXTS)
  ADD_TEST(test-smpi-mpich3-comm-raw       ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/comm ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests "-wrapper=${VALGRIND_WRAPPER}" -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/comm -tests=testlist -execarg=--cfg=contexts/factory:raw)
  SET_TESTS_PROPERTIES(test-smpi-mpich3-comm-raw PROPERTIES PASS_REGULAR_EXPRESSION "tests passed!")
endif()

foreach(file cmfree cmfree2 cmsplit2 cmsplit cmsplit_type commcreate1 comm_create_group comm_group_half comm_group_rand
        comm_info comm_info2 commname ctxalloc ctxsplit dup dupic dup_with_info ic1 ic2
        iccreate icgroup icm icsplit probe-intercomm comm_create_group_idup comm_idup_comm comm_idup_mul comm_idup comm_idup_iallreduce comm_idup_nb comm_idup_comm2 comm_idup_isend comm_idup_overlap
)
  set(examples_src ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/${file}.c)
endforeach()

set(examples_src ${examples_src}                                         PARENT_SCOPE)
set(txt_files    ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/testlist   PARENT_SCOPE)
