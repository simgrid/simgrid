set(bin_files  ${bin_files}  ${CMAKE_CURRENT_SOURCE_DIR}/generate_report  PARENT_SCOPE)

if("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_HOME_DIRECTORY}")
else()
    foreach(srcfile ${stdo_std_smpi})
        set(dstfile ${srcfile})
        string(REPLACE "${CMAKE_HOME_DIRECTORY}" "${CMAKE_BINARY_DIR}" dstfile "${dstfile}")
        #message("copy ${srcfile} to ${dstfile}")
        configure_file("${srcfile}" "${dstfile}" COPYONLY)
    endforeach()
endif()

set(txt_files  ${txt_files}  ${CMAKE_CURRENT_SOURCE_DIR}/README
                             ${CMAKE_CURRENT_SOURCE_DIR}/runtests
                             ${CMAKE_CURRENT_SOURCE_DIR}/testlist
                             ${CMAKE_CURRENT_SOURCE_DIR}/hostfile
                             ${CMAKE_CURRENT_SOURCE_DIR}/checktests
                             ${CMAKE_CURRENT_SOURCE_DIR}/generate_report
                             ${CMAKE_CURRENT_SOURCE_DIR}/util/dtypes.c
                             ${CMAKE_CURRENT_SOURCE_DIR}/util/mtest.c
                             ${CMAKE_CURRENT_SOURCE_DIR}/util/mtest_common.c
			     ${CMAKE_CURRENT_SOURCE_DIR}/util/mtestcheck.c
                             ${CMAKE_CURRENT_SOURCE_DIR}/util/mtest_datatype.c
                             ${CMAKE_CURRENT_SOURCE_DIR}/util/mtest_datatype_gen.c
                             ${CMAKE_CURRENT_SOURCE_DIR}/f77/testlist
                             ${CMAKE_CURRENT_SOURCE_DIR}/f90/testlist
                             ${CMAKE_CURRENT_SOURCE_DIR}/include/dtypes.h
                             ${CMAKE_CURRENT_SOURCE_DIR}/include/mpicolltest.h
                             ${CMAKE_CURRENT_SOURCE_DIR}/include/mpitestconf.h
                             ${CMAKE_CURRENT_SOURCE_DIR}/include/mpitestcxx.h
                             ${CMAKE_CURRENT_SOURCE_DIR}/include/mpitest.h
                             ${CMAKE_CURRENT_SOURCE_DIR}/include/mtest_datatype.h
                             ${CMAKE_CURRENT_SOURCE_DIR}/include/mpitest.h
                             ${CMAKE_CURRENT_SOURCE_DIR}/coll/allred_dtp.c
                             ${CMAKE_CURRENT_SOURCE_DIR}/coll/bcast_dtp.c
                             ${CMAKE_CURRENT_SOURCE_DIR}/coll/op_coll_dtp.c
                             ${CMAKE_CURRENT_SOURCE_DIR}/coll/reduce_dtp.c
                             ${CMAKE_CURRENT_SOURCE_DIR}/dtpools/dtpoolsconf.h
                             ${CMAKE_CURRENT_SOURCE_DIR}/dtpools/include/dtpools.h
                             ${CMAKE_CURRENT_SOURCE_DIR}/dtpools/include/dtpools_internal.h
                             ${CMAKE_CURRENT_SOURCE_DIR}/dtpools/src/dtpools.c
                             ${CMAKE_CURRENT_SOURCE_DIR}/dtpools/src/dtpools_attr.c
                             ${CMAKE_CURRENT_SOURCE_DIR}/dtpools/src/dtpools_custom.c
                             ${CMAKE_CURRENT_SOURCE_DIR}/dtpools/src/dtpools_desc.c
                             ${CMAKE_CURRENT_SOURCE_DIR}/dtpools/src/dtpools_init_verify.c
                             ${CMAKE_CURRENT_SOURCE_DIR}/dtpools/src/dtpools_misc.c
                             ${CMAKE_CURRENT_SOURCE_DIR}/include/mtest_common.h
                             ${CMAKE_CURRENT_SOURCE_DIR}/include/mtest_dtp.h
                             ${CMAKE_CURRENT_SOURCE_DIR}/io/setview_zero.c
                             ${CMAKE_CURRENT_SOURCE_DIR}/io/striped_hints.c
                             ${CMAKE_CURRENT_SOURCE_DIR}/io/zero_count.c
                             ${CMAKE_CURRENT_SOURCE_DIR}/pt2pt/pingping_dtp.c
                             ${CMAKE_CURRENT_SOURCE_DIR}/pt2pt/pipeline.c
                             ${CMAKE_CURRENT_SOURCE_DIR}/pt2pt/precv_anysrc.c
                             ${CMAKE_CURRENT_SOURCE_DIR}/pt2pt/precv_anysrc_exp.c
                             ${CMAKE_CURRENT_SOURCE_DIR}/pt2pt/recvnull.c
                             ${CMAKE_CURRENT_SOURCE_DIR}/pt2pt/rqfree.c
                             ${CMAKE_CURRENT_SOURCE_DIR}/pt2pt/send_tests.c
                            PARENT_SCOPE)

#build only once files used in each test (C version compiled here at root, F77 is in f77/util, and F90 in F90/util)

set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")

include_directories("${CMAKE_HOME_DIRECTORY}/include/smpi")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/dtpools/include/")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/dtpools/")

if(enable_smpi AND enable_testsuite_smpi_MPICH3)
  if(HAVE_PRIVATIZATION)
    add_library(mtest_c STATIC util/dtypes.c util/mtest.c  util/mtestcheck.c  util/mtest_datatype.c util/mtest_datatype_gen.c util/mtest_common.c)
    target_compile_options(mtest_c PRIVATE -Wno-error)
    add_library(dtpools STATIC dtpools/src/dtpools.c dtpools/src/dtpools_attr.c dtpools/src/dtpools_custom.c dtpools/src/dtpools_desc.c dtpools/src/dtpools_init_verify.c dtpools/src/dtpools_misc.c)
    target_include_directories(dtpools PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dtpools/include ${CMAKE_CURRENT_SOURCE_DIR}/dtpools)
    target_compile_options(dtpools PRIVATE -Wno-error)
    target_include_directories(mtest_c PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dtpools/include ${CMAKE_CURRENT_SOURCE_DIR}/dtpools)
  else()
    message(FATAL_ERROR "MPICH testsuite needs privatization. Please use a modern OS.")
  endif()
endif()

IF(enable_testsuite_smpi_MPICH3 AND SMPI_FORTRAN)
  ADD_TEST(test-smpi-mpich3-thread-f77     ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/f77/ ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests "-wrapper=${VALGRIND_WRAPPER}" -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/f77/ -tests=testlist -privatization=${HAVE_PRIVATIZATION} -execarg=--cfg=contexts/stack-size:8000 -execarg=--cfg=contexts/factory:thread -execarg=--cfg=smpi/privatization:${HAVE_PRIVATIZATION})
  SET_TESTS_PROPERTIES(test-smpi-mpich3-thread-f77 PROPERTIES PASS_REGULAR_EXPRESSION "tests passed!")
  ADD_TEST(test-smpi-mpich3-thread-f90     ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/f90/ ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests "-wrapper=${VALGRIND_WRAPPER}" -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/f90/ -tests=testlist -privatization=${HAVE_PRIVATIZATION} -execarg=--cfg=smpi/privatization:${HAVE_PRIVATIZATION} -execarg=--cfg=contexts/factory:thread)
  SET_TESTS_PROPERTIES(test-smpi-mpich3-thread-f90 PROPERTIES PASS_REGULAR_EXPRESSION "tests passed!")
ENDIF()
