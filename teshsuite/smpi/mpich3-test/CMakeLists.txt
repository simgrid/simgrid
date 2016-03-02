set(bin_files
  ${bin_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/generate_report
  PARENT_SCOPE)

if("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_HOME_DIRECTORY}")
else()
    foreach(srcfile ${stdo_std_smpi})
        set(dstfile ${srcfile})
        string(REPLACE "${CMAKE_HOME_DIRECTORY}" "${CMAKE_BINARY_DIR}" dstfile "${dstfile}")
        #message("copy ${srcfile} to ${dstfile}")
        configure_file("${srcfile}" "${dstfile}" COPYONLY)
    endforeach()
endif()

set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/README
  ${CMAKE_CURRENT_SOURCE_DIR}/runtests
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  ${CMAKE_CURRENT_SOURCE_DIR}/hostfile
  ${CMAKE_CURRENT_SOURCE_DIR}/checktests
  ${CMAKE_CURRENT_SOURCE_DIR}/generate_report
  ${CMAKE_CURRENT_SOURCE_DIR}/util/mtest.c
  ${CMAKE_CURRENT_SOURCE_DIR}/util/dtypes.c
  ${CMAKE_CURRENT_SOURCE_DIR}/util/mtest_manual.c
  ${CMAKE_CURRENT_SOURCE_DIR}/f77/testlist
  ${CMAKE_CURRENT_SOURCE_DIR}/f90/testlist
  ${CMAKE_CURRENT_SOURCE_DIR}/include/mpitestconf.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/dtypes.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/mpitest.h
  PARENT_SCOPE)


#build only once files used in each test (C version compiled here at root, F77 is in f77/util, and F90 in F90/util)

if(WIN32)
  set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
else()
  set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
endif()

include_directories("${CMAKE_HOME_DIRECTORY}/include/smpi")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/")

if(enable_smpi AND enable_smpi_MPICH3_testsuite)
#C version - use automatic privatization if mmap is supported, manual through SMPI macros if not
  if(HAVE_PRIVATIZATION)
    add_library(mtest_c STATIC util/mtest.c)
  else()
    add_library(mtest_c STATIC util/mtest_manual.c)
  endif()
endif()
