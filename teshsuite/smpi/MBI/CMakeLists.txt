# Copyright 2021-2022. The SimGrid Team. All rights reserved. 

# Integrates the MBI tests into the SimGrid build chain when asked to

# Only the python scripts are embeeded in the archive, and the C test files are generated at config time using these scripts.
# These python scripts are copied over from the MBI repository with as little changes as possible.

set(generator_scripts CollMatchingGenerator.py ResleakGenerator.py) # More generators to come

if (enable_smpi_MBI_testsuite)
  if (NOT enable_smpi)
    message(FATAL_ERROR "MBI test suite cannot be enabled without SMPI. Please change either setting.")
  endif()
  if (NOT enable_model-checking)
    message(FATAL_ERROR "MBI test suite cannot be enabled without the Mc SimGrid model-checker. Please change either setting.")
  endif()

  message(STATUS "Generating the MBI scripts")
  file(REMOVE_RECURSE  ${CMAKE_BINARY_DIR}/MBI/tmp)
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/MBI/tmp)
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/generator_utils.py DESTINATION ${CMAKE_BINARY_DIR}/MBI/tmp)
  foreach (script ${generator_scripts})
    message(STATUS "  $ ${CMAKE_CURRENT_SOURCE_DIR}/${script}")
    execute_process(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${script}
                    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/MBI/tmp)
  endforeach()

  set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  set(CMAKE_CXX_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicxx")
  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  # Connect the MBI tests to the other tests
  add_custom_target(tests-mbi COMMENT "Recompiling the MBI tests and tools.")
  add_dependencies(tests tests-mbi)

  file(GLOB cfiles RELATIVE ${CMAKE_BINARY_DIR}/MBI/tmp ${CMAKE_BINARY_DIR}/MBI/tmp/*.c )
  foreach(cfile ${cfiles})
    # Copy the generated files only if different
    file(COPY_FILE ${CMAKE_BINARY_DIR}/MBI/tmp/${cfile} ${CMAKE_BINARY_DIR}/MBI/${cfile} ONLY_IF_DIFFERENT)
    string(REGEX REPLACE "[.]c" "" basefile ${cfile})
    
    # Generate an executable for each of them
    add_executable(mbi_${basefile} EXCLUDE_FROM_ALL ${CMAKE_BINARY_DIR}/MBI/${cfile})
    target_link_libraries(mbi_${basefile} simgrid)
    target_compile_options(mbi_${basefile} PRIVATE "-Wno-error")
    set_target_properties(mbi_${basefile} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/MBI)
    add_dependencies(tests-mbi mbi_${basefile})

    # Generate a test case for each source file, using the MBI runner
    ADD_TEST(NAME mbi-${basefile}
             COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/MBI.py ${CMAKE_BINARY_DIR} ./mbi_${basefile} ${cfile}
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/MBI)
    SET_TESTS_PROPERTIES(mbi-${basefile}  PROPERTIES DEPENDS mbi-${basefile})
    SET_TESTS_PROPERTIES(mbi-${basefile}  PROPERTIES DEPENDS simgrid-mc)
  endforeach()

  if("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_HOME_DIRECTORY}")
  else()
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/MBIutils.py DESTINATION ${CMAKE_BINARY_DIR}/MBI) 
  endif()
endif()

# Add the needed files to the distribution
foreach(script ${generator_scripts})
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${script})
endforeach()

set(teshsuite_src ${teshsuite_src}
                  ${CMAKE_CURRENT_SOURCE_DIR}/generator_utils.py
                  ${CMAKE_CURRENT_SOURCE_DIR}/MBI.py
                  ${CMAKE_CURRENT_SOURCE_DIR}/MBIutils.py
                  ${CMAKE_CURRENT_SOURCE_DIR}/simgrid.py
                  PARENT_SCOPE)