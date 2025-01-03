if(enable_java)
  add_custom_target(java-all COMMENT "Building all Java examples...")
  add_dependencies(tests java-all)
  add_dependencies(java-all simgrid_jar)
  add_dependencies(java-all simgrid-java) # useful when the libs are not included in the jar
  add_dependencies(java-all simgrid)      # useful when the libs are not included in the jar
endif()

# comm_failure needs both exceptions and platform creation
# exec_dependent does not trigger the veto
foreach (example actor_daemon actor_exiting actor_join # actor_kill
                 actor_lifetime actor_migrate actor_suspend actor_yield
                 exec_async exec_basic exec_dependent
                 comm_await comm_awaituntil comm_dependent comm_host2host comm_pingpong comm_ready comm_suspend comm_throttling
                 mess_await
                 synchro_barrier synchro_condition_variable synchro_mutex synchro_semaphore
                 )
  
  # Use default source file unless specified otherwise
  if(NOT DEFINED _${example}_sources)
    set(_${example}_sources ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}.java)
  endif()
  # Use default tesh file unless specified otherwise
  if(NOT DEFINED _${example}_teshfile)
    set(_${example}_teshfile ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}.java.tesh)
  endif()

  if(enable_java AND (NOT DEFINED _${example}_disable))
    add_jar(java-${example} SOURCES ${_${example}_sources}
            INCLUDE_JARS simgrid_jar
            ENTRY_POINT ${example}
            OUTPUT_NAME ${example}
            OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${example_dir})
    add_dependencies(java-all java-${example})
    ADD_TESH(java-${example} --setenv LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/lib --setenv LC_CTYPE=C --setenv javacmd=${Java_JAVA_EXECUTABLE} --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv srcdir=${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}.java.tesh)
  endif()

  set(examples_src  ${examples_src}  ${_${example}_sources})
  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}.java.tesh)
endforeach()

set(examples_src  ${examples_src}                                                                          PARENT_SCOPE)
set(tesh_files    ${tesh_files}                                                                            PARENT_SCOPE)
set(xml_files     ${xml_files}    ${CMAKE_CURRENT_SOURCE_DIR}/actor_lifetime/actor_lifetime_d.xml
                  PARENT_SCOPE)
