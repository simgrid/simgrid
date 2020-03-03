foreach(x generic-simcalls stack-overflow)
  add_executable       (${x}  EXCLUDE_FROM_ALL ${x}/${x}.cpp)
  target_link_libraries(${x}  simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.cpp)
  add_dependencies(tests ${x})
endforeach()

set(teshsuite_src  ${teshsuite_src}                                                                        PARENT_SCOPE)
set(tesh_files     ${tesh_files}     
    ${CMAKE_CURRENT_SOURCE_DIR}/stack-overflow/stack-overflow.tesh  
    ${CMAKE_CURRENT_SOURCE_DIR}/generic-simcalls/generic-simcalls.tesh    
    PARENT_SCOPE)

if (NOT enable_memcheck AND NOT enable_address_sanitizer AND NOT enable_thread_sanitizer)
  ADD_TESH_FACTORIES(stack-overflow   "ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simix/stack-overflow --setenv srcdir=${CMAKE_HOME_DIRECTORY} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simix/stack-overflow stack-overflow.tesh)
  if (WIN32)
    SET_TESH_PROPERTIES(stack-overflow "ucontext;raw;boost" WILL_FAIL true)
  endif()
endif()
ADD_TESH_FACTORIES(generic-simcalls "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simix/generic-simcalls --setenv srcdir=${CMAKE_HOME_DIRECTORY} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simix/generic-simcalls generic-simcalls.tesh)

