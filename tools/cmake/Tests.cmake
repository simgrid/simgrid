SET(TESH_OPTION "--ignore-jenkins")
SET(TESH_COMMAND "${PYTHON_EXECUTABLE}" ${CMAKE_BINARY_DIR}/bin/tesh)

SET(TESH_LIBRARY_PATH "${CMAKE_BINARY_DIR}/lib")
if(NOT $ENV{LD_LIBRARY_PATH} STREQUAL "")
  SET(TESH_LIBRARY_PATH "${TESH_LIBRARY_PATH}:$ENV{LD_LIBRARY_PATH}")
endif()

IF(enable_memcheck)
  INCLUDE(FindValgrind)

  if (NOT VALGRIND_EXE MATCHES "NOTFOUND")
    execute_process(COMMAND ${VALGRIND_EXE} --version  OUTPUT_VARIABLE "VALGRIND_VERSION")
    message(STATUS "Valgrind version: ${VALGRIND_VERSION}")

    set(TESH_WRAPPER ${CMAKE_HOME_DIRECTORY}/tools/cmake/scripts/my_valgrind.pl)
    set(TESH_WRAPPER ${TESH_WRAPPER}\ --trace-children=yes\ --trace-children-skip=/usr/bin/*,/bin/*\ --leak-check=full\ --show-reachable=yes\ --track-origins=no\ --read-var-info=no\ --num-callers=20\ --suppressions=${CMAKE_HOME_DIRECTORY}/tools/simgrid.supp\ )
    if(enable_memcheck_xml)
      SET(TESH_WRAPPER ${TESH_WRAPPER}\ --xml=yes\ --xml-file=memcheck_test_%p.memcheck\ --child-silent-after-fork=yes\ )
    endif()
    set(TESH_OPTION ${TESH_OPTION} --setenv VALGRIND_NO_LEAK_CHECK=--leak-check=no\ --show-leak-kinds=none)

#    message(STATUS "tesh wrapper: ${TESH_WRAPPER}")

    mark_as_advanced(TESH_WRAPPER)
  else()
    set(enable_memcheck false)
    message(STATUS "Error: Command valgrind not found --> enable_memcheck autoset to false.")
  endif()
ENDIF()
SET(TESH_WRAPPER_UNBOXED "${TESH_WRAPPER}")
SEPARATE_ARGUMENTS(TESH_WRAPPER_UNBOXED)

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

# Build a list variable named FACTORIES_LIST with the given arguments, but:
# - replace wildcard "*" with all known factories
# - if the list begins with "^", take the complement
# - finally remove unsupported factories
#
# Used by ADD_TESH_FACTORIES, and SET_TESH_PROPERTIES
MACRO(SETUP_FACTORIES_LIST)
  set(ALL_KNOWN_FACTORIES "thread;boost;raw;ucontext")

  if("${ARGV}" STREQUAL "*")    # take all known factories
    SET(FACTORIES_LIST ${ALL_KNOWN_FACTORIES})
  elseif("${ARGV}" MATCHES "^\\^") # exclude given factories
    SET(FACTORIES_LIST ${ALL_KNOWN_FACTORIES})
    STRING(SUBSTRING "${ARGV}" 1 -1 EXCLUDED)
    LIST(REMOVE_ITEM FACTORIES_LIST ${EXCLUDED})
  else()                        # take given factories
    SET(FACTORIES_LIST "${ARGV}")
  endif()

  # Exclude unsupported factories. Threads are always available, thanks to C++11 threads.
  if(NOT HAVE_BOOST_CONTEXTS)
    LIST(REMOVE_ITEM FACTORIES_LIST "boost")
  endif()
  if(NOT HAVE_RAW_CONTEXTS)
    LIST(REMOVE_ITEM FACTORIES_LIST "raw")
  endif()
  if(NOT HAVE_UCONTEXT_CONTEXTS)
    LIST(REMOVE_ITEM FACTORIES_LIST "ucontext")
  endif()

  # Check that there is no unknown factory
  FOREACH(FACTORY ${FACTORIES_LIST})
    if(NOT FACTORY IN_LIST ALL_KNOWN_FACTORIES)
      message(FATAL_ERROR "Unknown factory: ${FACTORY}")
    endif()
  ENDFOREACH()
ENDMACRO()

MACRO(ADD_TESH_FACTORIES NAME FACTORIES)
  SET(ARGR ${ARGV})
  LIST(REMOVE_AT ARGR 0) # remove name
  FOREACH(I ${FACTORIES}) # remove all factories
    LIST(REMOVE_AT ARGR 0)
  ENDFOREACH()
  SETUP_FACTORIES_LIST(${FACTORIES})
  FOREACH(FACTORY ${FACTORIES_LIST})
    ADD_TESH("${NAME}-${FACTORY}" "--cfg" "contexts/factory:${FACTORY}" ${ARGR})
  ENDFOREACH()
ENDMACRO()

MACRO(SET_TESH_PROPERTIES NAME FACTORIES)
  SET(ARGR ${ARGV})
  LIST(REMOVE_AT ARGR 0) # remove name
  FOREACH(I ${FACTORIES}) # remove all factories
    LIST(REMOVE_AT ARGR 0)
  ENDFOREACH()
  SETUP_FACTORIES_LIST(${FACTORIES})
  FOREACH(FACTORY ${FACTORIES_LIST})
    set_tests_properties("${NAME}-${FACTORY}" PROPERTIES ${ARGR})
  ENDFOREACH()
ENDMACRO()

IF(enable_java)
  IF(WIN32)
    SET(TESH_CLASSPATH "${CMAKE_BINARY_DIR}/examples/deprecated/java/\;${CMAKE_BINARY_DIR}/teshsuite/java/\;${SIMGRID_JAR}")
    STRING(REPLACE "\;" "§" TESH_CLASSPATH "${TESH_CLASSPATH}")
  ELSE()
    SET(TESH_CLASSPATH "${CMAKE_BINARY_DIR}/examples/deprecated/java/:${CMAKE_BINARY_DIR}/teshsuite/java/:${SIMGRID_JAR}")
  ENDIF()
ENDIF()

# New tests should use the Catch Framework
set(UNIT_TESTS  src/xbt/unit-tests_main.cpp
                src/kernel/resource/NetworkModelIntf_test.cpp
                src/kernel/resource/profile/Profile_test.cpp
                src/kernel/routing/DijkstraZone_test.cpp
                src/kernel/routing/DragonflyZone_test.cpp
                src/kernel/routing/FatTreeZone_test.cpp
                src/kernel/routing/FloydZone_test.cpp
                src/kernel/routing/FullZone_test.cpp
                src/kernel/routing/StarZone_test.cpp
                src/kernel/routing/TorusZone_test.cpp
                src/surf/SplitDuplexLinkImpl_test.cpp
                src/xbt/config_test.cpp
                src/xbt/dict_test.cpp
                src/xbt/dynar_test.cpp
		src/xbt/random_test.cpp
                src/xbt/xbt_str_test.cpp
		src/kernel/lmm/maxmin_test.cpp)
if (SIMGRID_HAVE_MC)
  set(UNIT_TESTS ${UNIT_TESTS} src/mc/sosp/Snapshot_test.cpp src/mc/sosp/PageStore_test.cpp)
else()
  set(EXTRA_DIST ${EXTRA_DIST} src/mc/sosp/Snapshot_test.cpp src/mc/sosp/PageStore_test.cpp)
endif()
set(EXTRA_DIST ${EXTRA_DIST} src/kernel/routing/NetZone_test.hpp)

add_executable       (unit-tests EXCLUDE_FROM_ALL ${UNIT_TESTS})
add_dependencies     (tests unit-tests)
target_link_libraries(unit-tests simgrid)
ADD_TEST(unit-tests ${CMAKE_BINARY_DIR}/unit-tests)
set_property(TARGET unit-tests APPEND PROPERTY INCLUDE_DIRECTORIES "${INTERNAL_INCLUDES}")
set(EXTRA_DIST ${EXTRA_DIST} ${UNIT_TESTS})

unset(UNIT_TESTS)
