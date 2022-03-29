# Copyright 2021-2022. The SimGrid Team. All rights reserved.

# Integrates the MBI tests into the SimGrid build chain when asked to

# Only the python scripts are embeeded in the archive, and the C test files are generated at config time using these scripts.
# These python scripts are copied over from the MBI repository with as little changes as possible.

set(generator_scripts 
    CollArgGenerator.py
    CollComGenerator.py
    CollLocalConcurrencyGenerator.py
    CollMatchingGenerator.py
    CollP2PMatchingGenerator.py
    CollP2PMessageRaceGenerator.py
    CollTopoGenerator.py
    MissingWaitandStartGenerator.py
    P2PArgGenerator.py
    P2PComGenerator.py
    P2PInvalidComGenerator.py
    P2PLocalConcurrencyGenerator.py
    P2PMatchingANYSRCGenerator.py
    P2PMatchingGenerator.py
    P2PProbeGenerator.py
    ResleakGenerator.py
    RMAArgGenerator.py
    RMAInvalidArgGenerator.py
    RMALocalLocalConcurrencyGenerator.py
    RMAP2PGlobalConcurrencyGenerator.py
    RMARemoteLocalConcurrencyGenerator.py
    RMARemoteRemoteConcurrencyGenerator.py
    RMAReqLifecycleGenerator.py
    RMAWinBufferGenerator.py)

if (enable_smpi_MBI_testsuite)
  if (NOT enable_smpi)
    message(FATAL_ERROR "MBI test suite cannot be enabled without SMPI. Please change either setting.")
  endif()
  if (NOT enable_model-checking)
    message(FATAL_ERROR "MBI test suite cannot be enabled without the Mc SimGrid model-checker. Please change either setting.")
  endif()

  message(STATUS "Generating the MBI test cases")
  file(REMOVE_RECURSE  ${CMAKE_BINARY_DIR}/MBI/tmp)
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/MBI/tmp)
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/generator_utils.py DESTINATION ${CMAKE_BINARY_DIR}/MBI/tmp)
  foreach (script ${generator_scripts})
    message(STATUS "  $ ${CMAKE_CURRENT_SOURCE_DIR}/${script}")
    execute_process(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${script}
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

  # The following tests are known to fail because simgrid does not intercept local modifications yet
  # An idea could be to use ASan on the verified application, along with https://github.com/google/sanitizers/wiki/AddressSanitizerManualPoisoning
  # But currently, ASan is not usable at all, since the Checker dislikes this trick when it tries to read the memory of the app. 
  # We should change the checker to not read the app when verifying safty properties
  foreach(localmodif 
          LocalConcurrency_Iallgather_nok      LocalConcurrency_Iallgatherv_nok     LocalConcurrency_Iallreduce_nok      LocalConcurrency_Ialltoall_nok
          LocalConcurrency_Ialltoallv_nok      LocalConcurrency_Ibcast_nok          LocalConcurrency_Iexscan_nok         LocalConcurrency_Igather_nok
          LocalConcurrency_Irecv_Isend_nok     LocalConcurrency_Irecv_Send_init_nok LocalConcurrency_Irecv_Send_nok      LocalConcurrency_Ireduce_nok
          LocalConcurrency_Iscan_nok           LocalConcurrency_Iscatter_nok        LocalConcurrency_Recv_Isend_nok      LocalConcurrency_Recv_Send_init_nok
          LocalConcurrency_Recv_init_Isend_nok LocalConcurrency_Recv_init_Send_nok  LocalConcurrency_Recv_init_Send_init_nok

          GlobalConcurrency_Get_Isend_Irecv_nok GlobalConcurrency_Get_Isend_Recv_nok GlobalConcurrency_Get_Send_Irecv_nok GlobalConcurrency_Get_Send_Recv_nok 
          GlobalConcurrency_Put_Isend_Irecv_nok GlobalConcurrency_Put_Isend_Recv_nok GlobalConcurrency_Put_Send_Irecv_nok GlobalConcurrency_Put_Send_Recv_nok 

          GlobalConcurrency_rl_Win_fence_Get_rload_nok     GlobalConcurrency_rl_Win_fence_Get_rstore_nok 
          GlobalConcurrency_rl_Win_fence_Put_rload_nok     GlobalConcurrency_rl_Win_fence_Put_rstore_nok
          GlobalConcurrency_rl_Win_lock_all_Get_rload_nok  GlobalConcurrency_rl_Win_lock_all_Get_rstore_nok
          GlobalConcurrency_rl_Win_lock_all_Put_rload_nok  GlobalConcurrency_rl_Win_lock_all_Put_rstore_nok
       )
    set_tests_properties(mbi-${localmodif} PROPERTIES WILL_FAIL true)
  endforeach(localmodif )
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
