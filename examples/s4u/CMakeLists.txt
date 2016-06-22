foreach (example basic io mutex actions-comm)
  add_executable       (s4u_${example}  ${example}/s4u_${example}.cpp)
  target_link_libraries(s4u_${example}  simgrid)
  set_target_properties(s4u_${example}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${example})

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${example}/s4u_${example}.tesh)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/${example}/s4u_${example}.cpp)
endforeach()

add_executable       (s4u_basic_function  basic/s4u_basic_function.cpp)
target_link_libraries(s4u_basic_function  simgrid)
set_target_properties(s4u_basic_function  PROPERTIES RUNTIME_OUTPUT_DIRECTORY
 ${CMAKE_CURRENT_BINARY_DIR}/basic)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/basic/s4u_basic_function.cpp
                                   ${CMAKE_CURRENT_SOURCE_DIR}/basic/s4u_basic.h)

set(examples_src  ${examples_src}                                     PARENT_SCOPE)
set(tesh_files    ${tesh_files}                                       PARENT_SCOPE)
set(xml_files     ${xml_files}    ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/s4u_actions-comm_split_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/s4u_actions-comm_d.xml         PARENT_SCOPE)
set(txt_files     ${txt_files}    ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/s4u_actions-comm_split_p0.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/s4u_actions-comm_split_p1.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/s4u_actions-comm.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/README                                      PARENT_SCOPE)

foreach(example basic io mutex actions-comm)
  ADD_TESH_FACTORIES(s4u-${example} "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_CURRENT_BINARY_DIR}/${example} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_HOME_DIRECTORY}/examples/s4u/${example} s4u_${example}.tesh)
endforeach()
