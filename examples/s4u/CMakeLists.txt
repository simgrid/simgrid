foreach (example basic io)
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
