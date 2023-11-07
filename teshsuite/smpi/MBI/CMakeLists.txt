# Copyright 2021-2023. The SimGrid Team. All rights reserved.

# Integrates the MBI tests into the SimGrid build chain when asked to

# Only the python scripts are embeeded in the archive, and the C test files are generated at config time using these scripts.
# These python scripts are copied over from the MBI repository with as little changes as possible.

file(GLOB generator_scripts *Generator.py)

if (enable_testsuite_smpi_MBI)
  if (NOT enable_smpi)
    message(FATAL_ERROR "The MBI test suite cannot be enabled without SMPI. Please change either setting.")
  endif()
  if (NOT SIMGRID_HAVE_MC)
    message(FATAL_ERROR "The MBI test suite cannot be enabled without the model-checker. Please change either setting.")
  endif()

  message(STATUS "Generating the MBI test cases")
  file(REMOVE_RECURSE  ${CMAKE_BINARY_DIR}/MBI/tmp)
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/MBI/tmp)
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/generator_utils.py DESTINATION ${CMAKE_BINARY_DIR}/MBI/tmp)
  foreach (script ${generator_scripts})
    message(STATUS "  $ ${CMAKE_CURRENT_SOURCE_DIR}/${script}")
    execute_process(COMMAND ${PYTHON_EXECUTABLE} ${script}
                    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/MBI/tmp
                    RESULT_VARIABLE status)
    if (NOT status EQUAL 0)
      message(FATAL_ERROR "Command failed with status: ${status}")
    endif()
  endforeach()

  set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  set(CMAKE_CXX_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicxx")
  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  # Connect the MBI tests to the other tests
  add_custom_target(tests-mbi COMMENT "Recompiling the MBI tests and tools.")
  add_dependencies(tests tests-mbi)
  add_dependencies(tests-mbi simgrid-mc smpimain)

  # Remove Concurrency tests that are out of reach because SimGrid does not intercept local modifications yet
  # An idea could be to use ASan on the verified application, along with https://github.com/google/sanitizers/wiki/AddressSanitizerManualPoisoning
  # But currently, ASan is not usable at all, since the Checker dislikes this trick when it tries to read the memory of the app.
  # We should change the checker to not read the app when verifying safty properties
  file(GLOB cfiles ${CMAKE_BINARY_DIR}/MBI/tmp/LocalConcurrency*.c ${CMAKE_BINARY_DIR}/MBI/tmp/GlobalConcurrency*.c )
  foreach(cfile ${cfiles})
    file(REMOVE ${cfile})
  endforeach()
  list(LENGTH cfiles len)
  message(STATUS "Removed ${len} concurrency tests that would fail because we cannot intercept modifications of local variables.")
  unset(len)

  file(GLOB cfiles RELATIVE ${CMAKE_BINARY_DIR}/MBI/tmp ${CMAKE_BINARY_DIR}/MBI/tmp/*.c )
  foreach(cfile ${cfiles})
    # Copy the generated files only if different (needs cmake ≥ 3.21)
    if (CMAKE_VERSION VERSION_LESS 3.21)
      file(COPY ${CMAKE_BINARY_DIR}/MBI/tmp/${cfile} DESTINATION ${CMAKE_BINARY_DIR}/MBI/)
    else()
      file(COPY_FILE ${CMAKE_BINARY_DIR}/MBI/tmp/${cfile} ${CMAKE_BINARY_DIR}/MBI/${cfile} ONLY_IF_DIFFERENT)
    endif()
    string(REGEX REPLACE "[.]c" "" basefile ${cfile})

    # Generate an executable for each of them
    add_executable(mbi_${basefile} EXCLUDE_FROM_ALL ${CMAKE_BINARY_DIR}/MBI/${cfile})
    target_link_libraries(mbi_${basefile} simgrid)
    target_compile_options(mbi_${basefile} PRIVATE "-Wno-unused-variable")
    target_compile_options(mbi_${basefile} PRIVATE "-Wno-unused-but-set-variable")
    set_target_properties(mbi_${basefile} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/MBI)
    add_dependencies(tests-mbi mbi_${basefile})

    # Generate a test case for each source file, using the MBI runner
    ADD_TEST(NAME mbi-${basefile}
             COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/MBI.py ${CMAKE_BINARY_DIR} ./mbi_${basefile} ${cfile}
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/MBI)
    SET_TESTS_PROPERTIES(mbi-${basefile}  PROPERTIES DEPENDS mbi-${basefile})
    SET_TESTS_PROPERTIES(mbi-${basefile}  PROPERTIES DEPENDS simgrid-mc)
  endforeach()
  file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/MBI/tmp) # Clean temp files

  if("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_HOME_DIRECTORY}")
  else()
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/MBIutils.py DESTINATION ${CMAKE_BINARY_DIR}/MBI)
  endif()
endif()

# Add the needed files to the distribution
foreach(script ${generator_scripts})
  set(teshsuite_src ${teshsuite_src} ${script})
endforeach()

set(teshsuite_src ${teshsuite_src}
                  ${CMAKE_CURRENT_SOURCE_DIR}/generator_utils.py
                  ${CMAKE_CURRENT_SOURCE_DIR}/MBI.py
                  ${CMAKE_CURRENT_SOURCE_DIR}/MBIutils.py
                  ${CMAKE_CURRENT_SOURCE_DIR}/simgrid.py
                  PARENT_SCOPE)
