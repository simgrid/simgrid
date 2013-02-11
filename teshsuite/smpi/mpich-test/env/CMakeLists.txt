cmake_minimum_required(VERSION 2.6)

if(enable_smpi)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
  set(MPICH_FLAGS "-DHAVE_STDLIB_H=1 -DHAVE_UNISTD_H=1 -DHAVE_STRING_H=1 -DUSE_STDARG=1 -DHAVE_LONG_DOUBLE=1 -DHAVE_PROTOTYPES=1 -DHAVE_SIGNAL_H=1 -DHAVE_SIGACTION=1 -DHAVE_SLEEP=1 -DHAVE_SYSCONF=1")

  include_directories("${CMAKE_HOME_DIRECTORY}/include/smpi")
  
  add_executable(init init.c )
  add_executable(timers timers.c test.c)
  add_executable(timertest timertest.c test.c)
  add_executable(baseattr baseattr.c test.c)
  add_executable(gtime gtime.c test.c)
  add_executable(errhand errhand.c test.c)
  if(NOT WIN32)
    add_executable(sigchk sigchk.c test.c)
  endif()
  add_executable(aborttest aborttest.c)
  add_executable(testerr testerr.c)
  add_executable(getproc getproc.c)
  if(SMPI_F2C)
#    add_executable(baseattrf baseattrf.f)
#    add_executable(errhandf errhandf.f)
#    add_executable(errstringsf errstringsf.f)
    add_executable(getprocf getprocf.f)
  endif()
  target_link_libraries(init simgrid smpi )
  target_link_libraries(timers simgrid smpi )
  target_link_libraries(timertest simgrid smpi )
  target_link_libraries(baseattr simgrid smpi )
  target_link_libraries(gtime simgrid smpi )
  target_link_libraries(errhand simgrid smpi )
  if(NOT WIN32)
    target_link_libraries(sigchk simgrid smpi )
  endif()
	target_link_libraries(aborttest simgrid smpi )
  target_link_libraries(testerr simgrid smpi )
  target_link_libraries(getproc simgrid smpi )
  if(SMPI_F2C)
#    target_link_libraries(baseattrf simgrid smpi )
#    target_link_libraries(errhandf simgrid smpi )
#    target_link_libraries(errstringsf simgrid smpi )
    target_link_libraries(getprocf simgrid smpi )
  endif()

  set_target_properties(timers PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(timers PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(timertest PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(baseattr PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(gtime PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(errhand PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  if(NOT WIN32)
    set_target_properties(sigchk PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  endif()
  set_target_properties(aborttest PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(testerr PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(getproc PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  if(SMPI_F2C)
#    set_target_properties(baseattrf PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
#    set_target_properties(errhandf PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
#    set_target_properties(errstringsf PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
    set_target_properties(getprocf PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  endif()
endif()

set(tesh_files
  ${tesh_files}
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/test.c
  ${CMAKE_CURRENT_SOURCE_DIR}/init.c
  ${CMAKE_CURRENT_SOURCE_DIR}/timers.c
  ${CMAKE_CURRENT_SOURCE_DIR}/timertest.c
  ${CMAKE_CURRENT_SOURCE_DIR}/baseattr.c
  ${CMAKE_CURRENT_SOURCE_DIR}/gtime.c
  ${CMAKE_CURRENT_SOURCE_DIR}/errhand.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sigchk.c
  ${CMAKE_CURRENT_SOURCE_DIR}/aborttest.c
  ${CMAKE_CURRENT_SOURCE_DIR}/testerr.c
  ${CMAKE_CURRENT_SOURCE_DIR}/getproc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/test.h
  ${CMAKE_CURRENT_SOURCE_DIR}/baseattrf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/errhandf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/errstringsf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/getprocf.f
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/runtests
  ${CMAKE_CURRENT_SOURCE_DIR}/aborttest.std
  ${CMAKE_CURRENT_SOURCE_DIR}/errstringsf.std
  ${CMAKE_CURRENT_SOURCE_DIR}/env.std
  PARENT_SCOPE
  )
