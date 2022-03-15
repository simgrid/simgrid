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
                             ${CMAKE_CURRENT_SOURCE_DIR}/include/mpithreadtest.h
                             ${CMAKE_CURRENT_SOURCE_DIR}/include/mtest_datatype.h
                             ${CMAKE_CURRENT_SOURCE_DIR}/include/mpitest.h                            PARENT_SCOPE)

#build only once files used in each test (C version compiled here at root, F77 is in f77/util, and F90 in F90/util)

if(WIN32)
  set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
else()
  set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
endif()

include_directories("${CMAKE_HOME_DIRECTORY}/include/smpi")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/")

if(enable_smpi AND enable_smpi_MPICH3_testsuite)
  if(HAVE_PRIVATIZATION)
    add_library(mtest_c STATIC util/dtypes.c util/mtest.c  util/mtestcheck.c  util/mtest_datatype.c util/mtest_datatype_gen.c)
  else()
    message(FATAL_ERROR "MPICH testsuite needs privatization. Use a modern OS.")
  endif()
endif()

IF(enable_smpi_MPICH3_testsuite AND SMPI_FORTRAN)
  ADD_TEST(test-smpi-mpich3-thread-f77     ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/f77/ ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests "-wrapper=${VALGRIND_WRAPPER}" -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/f77/ -tests=testlist -privatization=${HAVE_PRIVATIZATION} -execarg=--cfg=contexts/stack-size:8000 -execarg=--cfg=contexts/factory:thread -execarg=--cfg=smpi/privatization:${HAVE_PRIVATIZATION})
  SET_TESTS_PROPERTIES(test-smpi-mpich3-thread-f77 PROPERTIES PASS_REGULAR_EXPRESSION "tests passed!")
  ADD_TEST(test-smpi-mpich3-thread-f90     ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/f90/ ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests "-wrapper=${VALGRIND_WRAPPER}" -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/f90/ -tests=testlist -privatization=${HAVE_PRIVATIZATION} -execarg=--cfg=smpi/privatization:${HAVE_PRIVATIZATION} -execarg=--cfg=contexts/factory:thread)
  SET_TESTS_PROPERTIES(test-smpi-mpich3-thread-f90 PROPERTIES PASS_REGULAR_EXPRESSION "tests passed!")
ENDIF()
