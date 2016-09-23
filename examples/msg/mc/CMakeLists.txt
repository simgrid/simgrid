foreach (x bugged1 bugged2 bugged3 centralized_mutex electric_fence bugged1_liveness bugged2_liveness)
  if(HAVE_MC)
    add_executable       (${x} ${x}.c)
    target_link_libraries(${x} simgrid)
  endif()
  set(examples_src  ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}.c)
  set(xml_files     ${xml_files}    ${CMAKE_CURRENT_SOURCE_DIR}/deploy_${x}.xml)
endforeach()

if(HAVE_MC)
  if(HAVE_C_STACK_CLEANER)
    add_executable       (bugged1_liveness_cleaner_on  bugged1_liveness.c)
    target_link_libraries(bugged1_liveness_cleaner_on  simgrid)
    set_target_properties(bugged1_liveness_cleaner_on  PROPERTIES COMPILE_FLAGS "-DGARBAGE_STACK -fstack-cleaner")
    
    add_executable       (bugged1_liveness_cleaner_off bugged1_liveness.c)
    target_link_libraries(bugged1_liveness_cleaner_off simgrid)
    set_target_properties(bugged1_liveness_cleaner_off PROPERTIES COMPILE_FLAGS "-DGARBAGE_STACK -fno-stack-cleaner")
  endif()
endif()

set(tesh_files   ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/bugged1.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/bugged2.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/bugged1_liveness.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/bugged1_liveness_visited.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/bugged1_liveness_sparse.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/bugged1_liveness_visited_sparse.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/centralized_mutex.tesh                PARENT_SCOPE)
set(xml_files    ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/deploy_bugged1_liveness_visited.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/platform.xml                          PARENT_SCOPE)
set(examples_src ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/bugged1_liveness.h
                                  ${CMAKE_CURRENT_SOURCE_DIR}/bugged2_liveness.h                    PARENT_SCOPE)
set(bin_files    ${bin_files}     ${CMAKE_CURRENT_SOURCE_DIR}/promela_bugged1_liveness
                                  ${CMAKE_CURRENT_SOURCE_DIR}/promela_bugged2_liveness
                                  ${CMAKE_CURRENT_SOURCE_DIR}/bugged1_liveness_stack_cleaner        PARENT_SCOPE)
