if(enable_smpi)
  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  add_library       (smpi_comm-dynamic-cost SHARED comm-dynamic-cost.cpp)
  target_link_libraries(smpi_comm-dynamic-cost simgrid)
  set_target_properties(smpi_comm-dynamic-cost
      PROPERTIES SUFFIX .so
  )
  if(enable_debug)
    ADD_TESH_FACTORIES(smpi_comm-dynamic-cost "*" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/smpi/comm_dynamic_costs --setenv libdir=${CMAKE_BINARY_DIR}/lib --cd ${CMAKE_BINARY_DIR}/examples/smpi/simple-execute ${CMAKE_HOME_DIRECTORY}/examples/smpi/comm_dynamic_costs/comm-dynamic-cost.tesh)
  else()
    message("-- Examples smpi_comm-dynamic-cost (built without debug).")
  endif()
  add_dependencies(tests smpi_comm-dynamic-cost)
  add_dependencies(smpi_comm-dynamic-cost smpi_simple-execute) #depends on simple-execute
endif()

set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/comm-dynamic-cost.tesh                   PARENT_SCOPE)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/comm-dynamic-cost.cpp                    PARENT_SCOPE)
set(bin_files    ${bin_files}      ${CMAKE_CURRENT_SOURCE_DIR}/hostfile                                 PARENT_SCOPE)
