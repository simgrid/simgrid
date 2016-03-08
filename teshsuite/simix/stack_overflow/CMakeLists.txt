add_executable       (stack_overflow stack_overflow.c)
target_link_libraries(stack_overflow simgrid)

ADD_TESH_FACTORIES(stack-overflow "thread;ucontext;raw" --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simix/stack_overflow --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simix/stack_overflow stack_overflow.tesh)

if(release AND (WIN32 OR CMAKE_SYSTEM_NAME MATCHES "Darwin"))
  foreach (factory raw thread ucontext)
    string (TOUPPER have_${factory}_contexts VARNAME)
    if (${VARNAME})
      SET_TESTS_PROPERTIES(stack-overflow-${factory} PROPERTIES WILL_FAIL true)
    endif()
  endforeach()
endif()

set(tesh_files     ${tesh_files}     ${CMAKE_CURRENT_SOURCE_DIR}/stack_overflow.tesh          PARENT_SCOPE)
set(xml_files      ${xml_files}      ${CMAKE_CURRENT_SOURCE_DIR}/stack_overflow.xml
                                     ${CMAKE_CURRENT_SOURCE_DIR}/stack_overflow_platform.xml  PARENT_SCOPE)
set(teshsuite_src  ${teshsuite_src}  ${CMAKE_CURRENT_SOURCE_DIR}/stack_overflow.c             PARENT_SCOPE)
