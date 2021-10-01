foreach(x context-defaults simcall-generic stack-overflow)
  add_executable       (${x}  EXCLUDE_FROM_ALL ${x}/${x}.cpp)
  target_link_libraries(${x}  simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
  set_property(TARGET ${x} APPEND PROPERTY INCLUDE_DIRECTORIES "${INTERNAL_INCLUDES}")
  add_dependencies(tests ${x})

  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.cpp)
endforeach()

## Add the easy tests: only simcall-generic for now
foreach(x simcall-generic)
  set(tesh_files    ${tesh_files} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  ADD_TESH_FACTORIES(tesh-${x} "*" --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/kernel/${x} --setenv srcdir=${CMAKE_HOME_DIRECTORY} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/kernel/${x} ${x}.tesh)
endforeach()

## Add the tests for stack-overflow
set(tesh_files    ${tesh_files} ${CMAKE_CURRENT_SOURCE_DIR}/stack-overflow/stack-overflow.tesh)
if (NOT enable_memcheck AND NOT enable_address_sanitizer AND NOT enable_thread_sanitizer)
  ADD_TESH_FACTORIES(stack-overflow   "^thread" --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/kernel/stack-overflow --setenv srcdir=${CMAKE_HOME_DIRECTORY} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/kernel/stack-overflow stack-overflow.tesh)
  if (WIN32)
    SET_TESH_PROPERTIES(stack-overflow "^thread" WILL_FAIL true)
  endif()
endif()

## Add the tests for context-default
foreach (factory raw thread boost ucontext)
  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/context-defaults/factory_${factory}.tesh)
endforeach()
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
set(teshsuite_src ${teshsuite_src}  ${CMAKE_CURRENT_SOURCE_DIR}/simcall-generic/blocking_simcall.hpp PARENT_SCOPE)
set(tesh_files    ${tesh_files}     PARENT_SCOPE)
set(xml_files     ${xml_files}      PARENT_SCOPE)
