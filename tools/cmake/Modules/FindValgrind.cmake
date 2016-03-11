find_program(VALGRIND_EXE
  NAME valgrind
  PATH_SUFFIXES bin/
  PATHS
  /opt
  /opt/local
  /opt/csw
  /sw
  /usr
  )

if(VALGRIND_EXE)
  message(STATUS "Found valgrind: ${VALGRIND_EXE}")
  SET(VALGRIND_COMMAND "${CMAKE_HOME_DIRECTORY}/tools/cmake/scripts/my_valgrind.pl")
  SET(MEMORYCHECK_COMMAND "${CMAKE_HOME_DIRECTORY}/tools/cmake/scripts/my_valgrind.pl")
endif()

if(enable_memcheck_xml)
  SET(VALGRIND_EXTRA_COMMAND_OPTIONS "--xml=yes --xml-file=memcheck_test_%p.memcheck --child-silent-after-fork=yes")
endif()

if(VALGRIND_EXE)
  execute_process(COMMAND ${VALGRIND_EXE} --version  OUTPUT_VARIABLE "VALGRIND_VERSION")
  string(REGEX MATCH "[0-9]+.[0-9]+.[0-9]+" NEW_VALGRIND_VERSION "${VALGRIND_VERSION}")
  if(NEW_VALGRIND_VERSION)
    message(STATUS "Valgrind version: ${NEW_VALGRIND_VERSION}")
    execute_process(COMMAND ${CMAKE_HOME_DIRECTORY}/tools/cmake/scripts/generate_memcheck_tests.pl ${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/tools/cmake/Tests.cmake OUTPUT_FILE ${CMAKE_BINARY_DIR}/memcheck_tests.cmake)
    set(MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --trace-children-skip=/usr/bin/*,/bin/* --leak-check=full --show-reachable=yes --track-origins=no --read-var-info=no --num-callers=20 --suppressions=${CMAKE_HOME_DIRECTORY}/tools/simgrid.supp ${VALGRIND_EXTRA_COMMAND_OPTIONS} ")
    message(STATUS "Valgrind options: ${MEMORYCHECK_COMMAND_OPTIONS}")
  else()
    set(enable_memcheck false)
    message(STATUS "Error: Command valgrind not found --> enable_memcheck autoset to false.")
  endif()
else()
  set(enable_memcheck false)
  message(FATAL_ERROR "Command valgrind not found --> enable_memcheck autoset to false.")
endif()

mark_as_advanced(VALGRIND_EXE)
