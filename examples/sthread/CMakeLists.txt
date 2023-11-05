set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Regular pthread examples: test the interceptor of the pthread interface
#########################################################################

foreach(x
        mutex-simple mutex-recursive
	producer-consumer)

  if("${CMAKE_SYSTEM}" MATCHES "Linux")
    add_executable       (pthread-${x} EXCLUDE_FROM_ALL pthread-${x}.c)
    set_target_properties(pthread-${x} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(pthread-${x} PRIVATE Threads::Threads)

    add_dependencies(tests pthread-${x})
    ADD_TESH_FACTORIES(pthread-${x} "^thread" --setenv libdir=${CMAKE_BINARY_DIR}/lib --cd ${CMAKE_BINARY_DIR}/examples/sthread ${CMAKE_CURRENT_SOURCE_DIR}/pthread-${x}.tesh)

    if(SIMGRID_HAVE_MC)
      add_dependencies(tests-mc pthread-${x})
      ADD_TESH_FACTORIES(pthread-mc-${x} "^thread" --setenv libdir=${CMAKE_BINARY_DIR}/lib --cd ${CMAKE_BINARY_DIR}/examples/sthread ${CMAKE_CURRENT_SOURCE_DIR}/pthread-mc-${x}.tesh)
    endif()
  endif()

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/pthread-${x}.tesh
                                     ${CMAKE_CURRENT_SOURCE_DIR}/pthread-mc-${x}.tesh)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/pthread-${x}.c)

endforeach()

# Regular pthread examples that may deadlock: test sthread + MC in that case
############################################################################

foreach(x
        mutex-simpledeadlock)

  if("${CMAKE_SYSTEM}" MATCHES "Linux") # sthread is linux-only

    add_executable       (pthread-${x} EXCLUDE_FROM_ALL pthread-${x}.c)
    set_target_properties(pthread-${x} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(pthread-${x} PRIVATE Threads::Threads)

    if(SIMGRID_HAVE_MC)
      add_dependencies(tests-mc pthread-${x})
      ADD_TESH_FACTORIES(pthread-mc-${x} "^thread" --setenv libdir=${CMAKE_BINARY_DIR}/lib --cd ${CMAKE_BINARY_DIR}/examples/sthread ${CMAKE_CURRENT_SOURCE_DIR}/pthread-mc-${x}.tesh)
    endif()
  endif()

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/pthread-mc-${x}.tesh)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/pthread-${x}.c)

endforeach()

# C++ tests with sthread+MC that live in their own directory
########################################################
foreach(example
        stdobject)

  if("${CMAKE_SYSTEM}" MATCHES "Linux") # sthread is linux-only

    add_executable       (${example} EXCLUDE_FROM_ALL ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}.cpp)
    set_target_properties(${example} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(${example} PRIVATE Threads::Threads)
    set_target_properties(${example} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${example})

    if(SIMGRID_HAVE_MC)
      add_dependencies(tests-mc ${example})
      ADD_TESH_FACTORIES(sthread-mc-${example} "^thread" --setenv libdir=${CMAKE_BINARY_DIR}/lib --cd ${CMAKE_BINARY_DIR}/examples/sthread/${example} ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}.tesh)
    endif()
  endif()

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}.tesh)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}.cpp)
endforeach()

# Regular sthread examples: test the internal interface for debugging purpose
#############################################################################

foreach(x
        mutex-simple)
  if("${CMAKE_SYSTEM}" MATCHES "Linux")
    add_executable       (sthread-${x} EXCLUDE_FROM_ALL sthread-${x}.c)
    set_target_properties(sthread-${x} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(sthread-${x} sthread)
    set_property(TARGET sthread-${x} APPEND PROPERTY INCLUDE_DIRECTORIES "${INTERNAL_INCLUDES}")

    add_dependencies(tests sthread-${x})
    ADD_TESH_FACTORIES(sthread-${x} "^thread" --cd ${CMAKE_BINARY_DIR}/examples/sthread ${CMAKE_CURRENT_SOURCE_DIR}/sthread-${x}.tesh)
  endif()

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/sthread-${x}.tesh)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/sthread-${x}.c)
endforeach()


# Add all extra files to the archive
####################################

set(tesh_files    ${tesh_files}     PARENT_SCOPE)
set(bin_files     ${bin_files}      PARENT_SCOPE)
set(examples_src  ${examples_src}   PARENT_SCOPE)
set(xml_files     ${xml_files}      PARENT_SCOPE)


