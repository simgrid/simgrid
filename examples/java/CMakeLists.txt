if(enable_java)
  add_custom_target(java-all COMMENT "Building all Java examples...")
  add_dependencies(tests java-all)
  add_dependencies(java-all simgrid_jar)
  add_dependencies(java-all simgrid-java) # useful when the libs are not included in the jar
  add_dependencies(java-all simgrid)      # useful when the libs are not included in the jar
endif()

# actor_exiting is not working yet
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/actor_exiting/actor_exiting.java)
set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/actor_exiting/actor_exiting.java.tesh)
# comm_failure needs both exceptions and platform creation
foreach (example exec_async exec_basic
                 comm_await comm_awaituntil comm_dependent comm_host2host comm_pingpong comm_ready comm_suspend comm_throttling)
  
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
            OUTPUT_NAME ${example}
            OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${example_dir})
    add_dependencies(java-all java-${example})
    ADD_TESH(java-${example}  --setenv LC_CTYPE=C --setenv javacmd=${Java_JAVA_EXECUTABLE} --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}.java.tesh)
  endif()

  set(examples_src  ${examples_src}  ${_${example}_sources})
  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}.java.tesh)
endforeach()

set(examples_src  ${examples_src}                                                                          PARENT_SCOPE)
set(tesh_files    ${tesh_files}                                                                            PARENT_SCOPE)
