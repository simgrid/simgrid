foreach (example basic io mutex)
  add_executable       (s4u_${example}  ${example}/s4u_${example}.cpp)
  target_link_libraries(s4u_${example}  simgrid)
  set_target_properties(s4u_${example}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${example})

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${example}/s4u_${example}.tesh)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/${example}/s4u_${example}.cpp)
endforeach()

set(examples_src  ${examples_src}                                     PARENT_SCOPE)
set(tesh_files    ${tesh_files}                                       PARENT_SCOPE)
set(txt_files     ${txt_files}    ${CMAKE_CURRENT_SOURCE_DIR}/README  PARENT_SCOPE)

ADD_TESH_FACTORIES(s4u-basic "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/examples/s4u/basic --cd ${CMAKE_HOME_DIRECTORY}/examples/s4u/basic s4u_basic.tesh)
ADD_TESH_FACTORIES(s4u-io    "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/examples/s4u/io    --cd ${CMAKE_HOME_DIRECTORY}/examples/s4u/io    s4u_io.tesh)
ADD_TESH_FACTORIES(s4u-mutex "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/examples/s4u/mutex    --cd ${CMAKE_HOME_DIRECTORY}/examples/s4u/mutex    s4u_mutex.tesh)

foreach(x actions-comm)
  add_executable       (s4u_${x}     ${x}/s4u_${x}.cpp)
  target_link_libraries(s4u_${x}     simgrid)
  set_target_properties(s4u_${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
  set(examples_src ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/s4u_${x}.cpp)
  set(tesh_files   ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/${x}/s4u_${x}.tesh)
endforeach()


foreach(x actions-comm)
  ADD_TESH_FACTORIES(s4u-${x} "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_CURRENT_BINARY_DIR}/${x} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_HOME_DIRECTORY}/examples/s4u/${x} s4u_${x}.tesh)
endforeach()
