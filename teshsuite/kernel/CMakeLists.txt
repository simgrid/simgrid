foreach(x context-stacksize)
  add_executable       (${x}  EXCLUDE_FROM_ALL ${x}/${x}.cpp)
  target_link_libraries(${x}  simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
  add_dependencies(tests ${x})

  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.cpp)
endforeach()

## Add the tests.
## Some need to be run with all factories, some don't need tesh to run

# Only with the raw contexts
ADD_TESH_FACTORIES(tesh-kern-context-stacksize "raw;boost;ucontext" --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/kernel/context-stacksize --setenv srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/kernel/context-stacksize --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_BINARY_DIR}/teshsuite/kernel/context-stacksize ${CMAKE_HOME_DIRECTORY}/teshsuite/kernel/context-stacksize/context-stacksize.tesh)

