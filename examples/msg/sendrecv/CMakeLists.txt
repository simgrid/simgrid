add_executable       (sendrecv      sendrecv.c)
target_link_libraries(sendrecv      simgrid)

ADD_TESH_FACTORIES(msg-sendrecv-CLM03 "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/sendrecv/sendrecv_CLM03.tesh)
ADD_TESH_FACTORIES(msg-sendrecv-Vegas "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/sendrecv/sendrecv_Vegas.tesh)
ADD_TESH_FACTORIES(msg-sendrecv-Reno  "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/sendrecv/sendrecv_Reno.tesh)

set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv_CLM03.tesh
                                   ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv_Reno.tesh
                                   ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv_Vegas.tesh      PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/deployment_sendrecv.xml  PARENT_SCOPE)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv.c               PARENT_SCOPE)
set(txt_files     ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/README                   PARENT_SCOPE)
