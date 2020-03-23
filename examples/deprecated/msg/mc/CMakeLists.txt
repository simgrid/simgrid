foreach (x bugged1 bugged2 bugged3 centralized_mutex bugged1_liveness bugged2_liveness)
  if(SIMGRID_HAVE_MC AND SIMGRID_HAVE_MSG)
    add_executable       (${x} EXCLUDE_FROM_ALL ${x}.c)
    target_link_libraries(${x} simgrid)
    add_dependencies(tests ${x})
  endif()
  set(examples_src  ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}.c)
  set(xml_files     ${xml_files}    ${CMAKE_CURRENT_SOURCE_DIR}/deploy_${x}.xml)
endforeach()

if(SIMGRID_HAVE_MC AND SIMGRID_HAVE_MSG)
  if(HAVE_C_STACK_CLEANER)
    add_executable       (bugged1_liveness_cleaner_on  EXCLUDE_FROM_ALL bugged1_liveness.c)
    target_link_libraries(bugged1_liveness_cleaner_on  simgrid)
    set_target_properties(bugged1_liveness_cleaner_on  PROPERTIES COMPILE_FLAGS "-DGARBAGE_STACK -fstack-cleaner")
    add_dependencies(tests bugged1_liveness_cleaner_on)
    
    add_executable       (bugged1_liveness_cleaner_off EXCLUDE_FROM_ALL bugged1_liveness.c)
    target_link_libraries(bugged1_liveness_cleaner_off simgrid)
    set_target_properties(bugged1_liveness_cleaner_off PROPERTIES COMPILE_FLAGS "-DGARBAGE_STACK -fno-stack-cleaner")
    add_dependencies(tests bugged1_liveness_cleaner_off)
  endif()

  ADD_TESH_FACTORIES(mc-bugged1    "ucontext;raw" --setenv bindir=${CMAKE_BINARY_DIR}/examples/deprecated/msg/mc
                                                  --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms 
                                                  --cd ${CMAKE_HOME_DIRECTORY}/examples/deprecated/msg/mc bugged1.tesh)
  ADD_TESH_FACTORIES(mc-bugged2    "ucontext;raw" --setenv bindir=${CMAKE_BINARY_DIR}/examples/deprecated/msg/mc 
                                                  --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms 
                                                  --cd ${CMAKE_HOME_DIRECTORY}/examples/deprecated/msg/mc bugged2.tesh)
  IF(HAVE_UCONTEXT_CONTEXTS AND SIMGRID_PROCESSOR_x86_64) # liveness model-checking works only on 64bits (for now ...)
    ADD_TESH(mc-bugged1-liveness-ucontext         --setenv bindir=${CMAKE_BINARY_DIR}/examples/deprecated/msg/mc 
                                                  --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms 
                                                  --cd ${CMAKE_HOME_DIRECTORY}/examples/deprecated/msg/mc bugged1_liveness.tesh)
    ADD_TESH(mc-bugged1-liveness-visited-ucontext --setenv bindir=${CMAKE_BINARY_DIR}/examples/deprecated/msg/mc 
                                                  --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms 
                                                  --cd ${CMAKE_HOME_DIRECTORY}/examples/deprecated/msg/mc bugged1_liveness_visited.tesh)
    IF(HAVE_C_STACK_CLEANER)
      # This test checks if the stack cleaner is making a difference:
      ADD_TEST(mc-bugged1-liveness-stack-cleaner ${CMAKE_HOME_DIRECTORY}/examples/deprecated/msg/mc/bugged1_liveness_stack_cleaner ${CMAKE_HOME_DIRECTORY}/examples/deprecated/msg/mc/ ${CMAKE_BINARY_DIR}/examples/deprecated/msg/mc/)
    ENDIF()
  ENDIF()

  if (enable_coverage)
    SET_TESTS_PROPERTIES(mc-bugged1-liveness-visited-ucontext PROPERTIES RUN_SERIAL "TRUE")
  endif()
endif()

set(tesh_files   ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/bugged1.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/bugged2.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/bugged1_liveness.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/bugged1_liveness_visited.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/centralized_mutex.tesh                PARENT_SCOPE)
set(xml_files    ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/deploy_bugged1_liveness_visited.xml   PARENT_SCOPE)
set(examples_src ${examples_src}                                                                    PARENT_SCOPE)
set(bin_files    ${bin_files}     ${CMAKE_CURRENT_SOURCE_DIR}/promela_bugged1_liveness
                                  ${CMAKE_CURRENT_SOURCE_DIR}/promela_bugged2_liveness
                                  ${CMAKE_CURRENT_SOURCE_DIR}/bugged1_liveness_stack_cleaner        PARENT_SCOPE)
