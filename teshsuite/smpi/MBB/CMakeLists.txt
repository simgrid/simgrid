# Copyright 2021-2026. The SimGrid Team. All rights reserved.

# Integrates the MPI-BugBench (MBB) tests into the SimGrid build chain when asked to.
# Only the generator is embedded in the archive (see README.md): the C test files are generated at config time.

set(MBB_LEVEL 2 CACHE STRING "Generation level of the MBB test cases, from 1 (130 cases) to 5 (way too many cases)")

if (enable_testsuite_smpi_MBB)
  if (NOT enable_smpi)
    message(FATAL_ERROR "The MBB test suite cannot be enabled without SMPI. Please change either setting.")
  endif()
  if (NOT SIMGRID_HAVE_MC)
    message(FATAL_ERROR "The MBB test suite cannot be enabled without the model-checker. Please change either setting.")
  endif()
  if (NOT MBB_LEVEL MATCHES "^[1-5]$")
    message(FATAL_ERROR "MBB_LEVEL must be between 1 and 5, not '${MBB_LEVEL}'.")
  endif()
  execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import pandas, numpy, tqdm, typing_extensions"
                  RESULT_VARIABLE status ERROR_QUIET)
  if (NOT status EQUAL 0)
    message(FATAL_ERROR "The MBB test suite needs the python modules pandas, numpy, tqdm and typing_extensions.")
  endif()

  # Levels 2 and 4 only generate the cases that are common in real-world codes, as scored in score_table.csv
  set(generate_args --generator_dir ${CMAKE_CURRENT_SOURCE_DIR}/upstream/errors --level ${MBB_LEVEL}
                    --outpath ${CMAKE_BINARY_DIR}/MBB/tmp)
  if (MBB_LEVEL EQUAL 2 OR MBB_LEVEL EQUAL 4)
    list(APPEND generate_args --score_table ${CMAKE_CURRENT_SOURCE_DIR}/score_table.csv)
  endif()

  message(STATUS "Generating the MBB test cases of level ${MBB_LEVEL}")
  file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/MBB/tmp)
  execute_process(COMMAND ${CMAKE_COMMAND} -E env PYTHONDONTWRITEBYTECODE=1
                          ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/upstream/generate.py ${generate_args}
                  OUTPUT_QUIET ERROR_VARIABLE errors RESULT_VARIABLE status)
  if (NOT status EQUAL 0)
    message(FATAL_ERROR "Generation of the MBB test cases failed with status ${status}:\n${errors}")
  endif()

  set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  set(CMAKE_CXX_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicxx")
  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  # Connect the MBB tests to the other tests
  add_custom_target(tests-mbb COMMENT "Recompiling the MBB tests and tools.")
  add_dependencies(tests tests-mbb)
  add_dependencies(tests-mbb simgrid-mc smpimain)

  # Remove Concurrency tests that are out of reach because SimGrid does not intercept local modifications yet.
  # An idea could be to extend the data race detection of src/mc/smemory (SlabTrackState): each pending MPI operation
  # would be a pseudo-thread accessing its buffer, joined into its caller upon completion (wait, test, fence, unlock).
  file(GLOB_RECURSE cfiles ${CMAKE_BINARY_DIR}/MBB/tmp/LocalConcurrency-*.c
                          ${CMAKE_BINARY_DIR}/MBB/tmp/GlobalConcurrency-*.c)
  foreach(cfile ${cfiles})
    file(REMOVE ${cfile})
  endforeach()
  list(LENGTH cfiles len)
  message(STATUS "Removed ${len} concurrency tests that would fail because we cannot intercept modifications of local variables.")
  unset(len)

  # The generator sorts the cases in subdirectories, but their names are unique
  file(GLOB_RECURSE cfiles RELATIVE ${CMAKE_BINARY_DIR}/MBB/tmp ${CMAKE_BINARY_DIR}/MBB/tmp/*.c)
  foreach(cfile ${cfiles})
    get_filename_component(basefile ${cfile} NAME_WE)
    # Copy the generated files only if different (needs cmake ≥ 3.21)
    if (CMAKE_VERSION VERSION_LESS 3.21)
      file(COPY ${CMAKE_BINARY_DIR}/MBB/tmp/${cfile} DESTINATION ${CMAKE_BINARY_DIR}/MBB/)
    else()
      file(COPY_FILE ${CMAKE_BINARY_DIR}/MBB/tmp/${cfile} ${CMAKE_BINARY_DIR}/MBB/${basefile}.c ONLY_IF_DIFFERENT)
    endif()

    # Generate an executable for each of them
    add_executable(mbb_${basefile} EXCLUDE_FROM_ALL ${CMAKE_BINARY_DIR}/MBB/${basefile}.c)
    target_link_libraries(mbb_${basefile} simgrid)
    target_compile_options(mbb_${basefile} PRIVATE "-Wno-unused-variable")
    target_compile_options(mbb_${basefile} PRIVATE "-Wno-unused-but-set-variable")
    set_target_properties(mbb_${basefile} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/MBB)
    add_dependencies(tests-mbb mbb_${basefile})

    # Generate a test case for each source file, using the MBB runner
    ADD_TEST(NAME mbb-${basefile}
             COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/MBB.py ${CMAKE_BINARY_DIR} ./mbb_${basefile} ${basefile}.c
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/MBB)
    SET_TESTS_PROPERTIES(mbb-${basefile} PROPERTIES DEPENDS simgrid-mc)
  endforeach()
  file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/MBB/tmp) # Clean temp files
endif()

# Add the needed files to the distribution
file(GLOB_RECURSE upstream_scripts ${CMAKE_CURRENT_SOURCE_DIR}/upstream/*.py)
set(teshsuite_src ${teshsuite_src} ${upstream_scripts}
                  ${CMAKE_CURRENT_SOURCE_DIR}/MBB.py
                  ${CMAKE_CURRENT_SOURCE_DIR}/README.md
                  ${CMAKE_CURRENT_SOURCE_DIR}/score_table.csv
                  PARENT_SCOPE)
