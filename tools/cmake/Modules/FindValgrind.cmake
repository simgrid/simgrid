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
mark_as_advanced(VALGRIND_EXE)

if(enable_memcheck)
  if (NOT VALGRIND_EXE MATCHES "NOTFOUND")
    execute_process(COMMAND ${VALGRIND_EXE} --version  OUTPUT_VARIABLE "VALGRIND_VERSION")
    message(STATUS "Valgrind version: ${VALGRIND_VERSION}")

    set(TESH_WRAPPER ${CMAKE_HOME_DIRECTORY}/tools/cmake/scripts/my_valgrind.pl)
    set(TESH_WRAPPER ${TESH_WRAPPER}\ --trace-children=yes\ --trace-children-skip=/usr/bin/*,/bin/*\ --leak-check=full\ --show-reachable=yes\ --track-origins=no\ --read-var-info=no\ --num-callers=20\ --suppressions=${CMAKE_HOME_DIRECTORY}/tools/simgrid.supp\ )
    if(enable_memcheck_xml)
      SET(TESH_WRAPPER ${TESH_WRAPPER}\ --xml=yes\ --xml-file=memcheck_test_%p.memcheck\ --child-silent-after-fork=yes\ )
    endif()

#    message(STATUS "tesh wrapper: ${TESH_WRAPPER}")

    mark_as_advanced(TESH_WRAPPER)
  else()
    set(enable_memcheck false)
    message(STATUS "Error: Command valgrind not found --> enable_memcheck autoset to false.")
  endif()
endif()
