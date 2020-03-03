foreach(x context-defaults context-stacksize)
  add_executable       (${x}  EXCLUDE_FROM_ALL ${x}/${x}.cpp)
  target_link_libraries(${x}  simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
  add_dependencies(tests ${x})

  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.cpp)
endforeach()

foreach (factory raw thread boost ucontext)
  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/context-defaults/factory_${factory}.tesh)
endforeach()

## Add the tests.
## Some need to be run with all factories, some don't need tesh to run

# This test should not be executed with thread contexts, as they ignore stack size changes
ADD_TESH_FACTORIES(tesh-kernel-context-stacksize "raw;boost;ucontext" --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/kernel/context-stacksize --setenv srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/kernel/context-stacksize --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_BINARY_DIR}/teshsuite/kernel/context-stacksize ${CMAKE_HOME_DIRECTORY}/teshsuite/kernel/context-stacksize/context-stacksize.tesh)
set(tesh_files    ${tesh_files} ${CMAKE_CURRENT_SOURCE_DIR}/context-stacksize/context-stacksize.tesh)

# Ensure that we have the right default setting for the context factory
IF(HAVE_RAW_CONTEXTS)
  ADD_TESH(tesh-kernel-context-default --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/kernel/context-defaults --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/kernel/context-defaults factory_raw.tesh)
ELSEIF(HAVE_UCONTEXT_CONTEXTS)
  ADD_TESH(tesh-kernel-context-default --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/kernel/context-defaults --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/kernel/context-defaults factory_ucontext.tesh)
ELSEIF(HAVE_BOOST_CONTEXTS)
  ADD_TESH(tesh-kernel-context-default --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/kernel/context-defaults --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/kernel/context-defaults factory_boost.tesh)
ELSE()
  ADD_TESH(tesh-kernel-context-default --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/kernel/context-defaults --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/kernel/context-defaults factory_thread.tesh)
ENDIF()
# Ensure that we get what we want when specifying the context factory on the command line
foreach (factory raw thread boost ucontext)
  string (TOUPPER have_${factory}_contexts VARNAME)
  if (${factory} STREQUAL "thread" OR ${VARNAME})
    # Cannot use ADD_TESH() here since ${factory} is part of the tesh filename
    ADD_TESH(tesh-kernel-factory-${factory} --cfg contexts/factory:${factory} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/kernel/context-defaults --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/kernel/context-defaults factory_${factory}.tesh)
  endif()
endforeach()


# Pack the files in the archive
set(teshsuite_src ${teshsuite_src}  PARENT_SCOPE)
set(tesh_files    ${tesh_files}     PARENT_SCOPE)
set(xml_files     ${xml_files}      PARENT_SCOPE)
