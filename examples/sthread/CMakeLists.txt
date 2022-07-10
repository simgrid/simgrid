set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Regular pthread examples: test the interceptor of the pthread interface
#########################################################################

foreach(x
        mutex-simple)

  if("${CMAKE_SYSTEM}" MATCHES "Linux")
    add_executable       (pthread-${x} EXCLUDE_FROM_ALL pthread-${x}.c)
    set_target_properties(pthread-${x} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(pthread-${x} PRIVATE Threads::Threads)
    target_link_options(pthread-${x} PUBLIC "-Wl,-znorelro -Wl,-znoseparate-code")

    add_dependencies(tests pthread-${x})
    ADD_TESH_FACTORIES(pthread-${x} "^thread" --setenv libdir=${CMAKE_BINARY_DIR}/lib --cd ${CMAKE_BINARY_DIR}/examples/sthread ${CMAKE_CURRENT_SOURCE_DIR}/pthread-${x}.tesh)
  endif()

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/pthread-${x}.tesh)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/pthread-${x}.c)

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
  endif()

#  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/sthread-${x}.tesh)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/sthread-${x}.c)
endforeach()


# Add all extra files to the archive
####################################

set(tesh_files    ${tesh_files}     PARENT_SCOPE)
set(bin_files     ${bin_files}      PARENT_SCOPE)
set(examples_src  ${examples_src}   PARENT_SCOPE)
set(xml_files     ${xml_files}      PARENT_SCOPE)


