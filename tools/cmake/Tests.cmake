IF(enable_smpi AND NOT WIN32)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpicc)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpicxx)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpiff)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpif90)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpirun)
ENDIF()

SET(TESH_OPTION "--ignore-jenkins")
SET(TESH_COMMAND "${PYTHON_EXECUTABLE}" ${CMAKE_BINARY_DIR}/bin/tesh)

IF(enable_memcheck)
  INCLUDE(FindValgrind)
ENDIF()

#some tests may take forever on non futexes systems, using busy_wait with n cores < n workers
# default to posix for these tests if futexes are not supported
IF(NOT HAVE_FUTEX_H)
  SET(CONTEXTS_SYNCHRO --cfg contexts/synchro:posix)
ENDIF()

MACRO(ADD_TESH NAME)
  SET(ARGT ${ARGV})
  LIST(REMOVE_AT ARGT 0)
  IF(WIN32)
    STRING(REPLACE "§" "\;" ARGT "${ARGT}")
  ENDIF()
  if(TESH_WRAPPER)
    ADD_TEST(${NAME} ${TESH_COMMAND} --wrapper "${TESH_WRAPPER}" ${TESH_OPTION} ${ARGT})
  else()
    ADD_TEST(${NAME} ${TESH_COMMAND} ${TESH_OPTION} ${ARGT})
  endif()
ENDMACRO()

MACRO(ADD_TESH_FACTORIES NAME FACTORIES)
  SET(ARGR ${ARGV})
  LIST(REMOVE_AT ARGR 0) # remove name
  FOREACH(I ${FACTORIES}) # remove all factories
    LIST(REMOVE_AT ARGR 0)
  ENDFOREACH()
  FOREACH(FACTORY ${FACTORIES})
    if ((${FACTORY} STREQUAL "thread" AND HAVE_THREAD_CONTEXTS) OR
        (${FACTORY} STREQUAL "boost" AND HAVE_BOOST_CONTEXTS) OR
        (${FACTORY} STREQUAL "raw" AND HAVE_RAW_CONTEXTS) OR
        (${FACTORY} STREQUAL "ucontext" AND HAVE_UCONTEXT_CONTEXTS))
      ADD_TESH("${NAME}-${FACTORY}" "--cfg" "contexts/factory:${FACTORY}" ${ARGR})
    ENDIF()
  ENDFOREACH()
ENDMACRO()

IF(enable_java)
  IF(WIN32)
    SET(TESH_CLASSPATH "${CMAKE_BINARY_DIR}/examples/java/\;${CMAKE_BINARY_DIR}/teshsuite/java/\;${SIMGRID_JAR}")
    STRING(REPLACE "\;" "§" TESH_CLASSPATH "${TESH_CLASSPATH}")
  ELSE()
    SET(TESH_CLASSPATH "${CMAKE_BINARY_DIR}/examples/java/:${CMAKE_BINARY_DIR}/teshsuite/java/:${SIMGRID_JAR}")
  ENDIF()
ENDIF()

IF(SIMGRID_HAVE_MC)
  ADD_TESH_FACTORIES(mc-bugged1                "ucontext;raw" --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/mc --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/mc bugged1.tesh)
  ADD_TESH_FACTORIES(mc-bugged2                "ucontext;raw" --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/mc --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/mc bugged2.tesh)
  IF(HAVE_UCONTEXT_CONTEXTS AND SIMGRID_PROCESSOR_x86_64) # liveness model-checking works only on 64bits (for now ...)
    ADD_TESH(mc-bugged1-liveness-ucontext         --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/mc --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/mc bugged1_liveness.tesh)
    ADD_TESH(mc-bugged1-liveness-ucontext-sparse  --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/mc --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/mc bugged1_liveness_sparse.tesh)
    ADD_TESH(mc-bugged1-liveness-visited-ucontext --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/mc --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/mc bugged1_liveness_visited.tesh)
    ADD_TESH(mc-bugged1-liveness-visited-ucontext-sparse --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/mc --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/mc bugged1_liveness_visited_sparse.tesh)
    IF(HAVE_C_STACK_CLEANER)
      # This test checks if the stack cleaner is making a difference:
      ADD_TEST(mc-bugged1-liveness-stack-cleaner ${CMAKE_HOME_DIRECTORY}/examples/msg/mc/bugged1_liveness_stack_cleaner ${CMAKE_HOME_DIRECTORY}/examples/msg/mc/ ${CMAKE_BINARY_DIR}/examples/msg/mc/)
    ENDIF()
  ENDIF()
ENDIF()

IF(enable_smpi_MPICH3_testsuite AND SMPI_FORTRAN AND HAVE_THREAD_CONTEXTS)
  ADD_TEST(test-smpi-mpich3-thread-f77     ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/f77/ ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests ${TESH_OPTION} -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/f77/ -tests=testlist -privatization=${HAVE_PRIVATIZATION} -execarg=--cfg=contexts/stack-size:8000 -execarg=--cfg=contexts/factory:thread -execarg=--cfg=smpi/privatization:${HAVE_PRIVATIZATION})
  SET_TESTS_PROPERTIES(test-smpi-mpich3-thread-f77 PROPERTIES PASS_REGULAR_EXPRESSION "tests passed!")
  ADD_TEST(test-smpi-mpich3-thread-f90     ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/f90/ ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests ${TESH_OPTION} -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/f90/ -tests=testlist -privatization=${HAVE_PRIVATIZATION} -execarg=--cfg=smpi/privatization:${HAVE_PRIVATIZATION} -execarg=--cfg=contexts/factory:thread)
  SET_TESTS_PROPERTIES(test-smpi-mpich3-thread-f90 PROPERTIES PASS_REGULAR_EXPRESSION "tests passed!")
ENDIF()

IF(SIMGRID_HAVE_LUA)
  # Tests testing simulation from C but using lua for platform files. Executed like this
  # ~$ ./masterslave platform.lua deploy.lua
  ADD_TESH(lua-platform-masterslave                --setenv srcdir=${CMAKE_HOME_DIRECTORY} --setenv bindir=${CMAKE_BINARY_DIR} --cd ${CMAKE_BINARY_DIR} ${CMAKE_HOME_DIRECTORY}/teshsuite/lua/lua_platforms.tesh)
  SET_TESTS_PROPERTIES(lua-platform-masterslave    PROPERTIES ENVIRONMENT "LUA_CPATH=${CMAKE_BINARY_DIR}/lib/lib?.${LIB_EXE}")
ENDIF()

ADD_TEST(testall                                 ${CMAKE_BINARY_DIR}/testall)

# New tests should use the Boost Unit Test Framework


if(Boost_UNIT_TEST_FRAMEWORK_FOUND)
  add_executable       (unit_tmgr src/surf/trace_mgr_test.cpp)
  target_link_libraries(unit_tmgr simgrid ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
  ADD_TEST(unit_tmgr ${CMAKE_BINARY_DIR}/unit_tmgr --build_info=yes)
  
else()
  set(EXTRA_DIST       ${EXTRA_DIST}       src/surf/trace_mgr_test.cpp)
endif()
