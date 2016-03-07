foreach (example masterslave_failure masterslave_forwarder masterslave_mailbox)
  add_executable       (${example} ${example}.c)
  target_link_libraries(${example} simgrid)
  set(examples_src  ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${example}.c)
  set(tesh_files  ${tesh_files} ${CMAKE_CURRENT_SOURCE_DIR}/${example}.tesh 
                                ${CMAKE_CURRENT_SOURCE_DIR}/${example}_crosstraffic.tesh)

  ADD_TESH_FACTORIES(msg-${example} "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/masterslave/${example}_crosstraffic.tesh)
  ADD_TESH_FACTORIES(msg-${example}-no-crosstraffic   "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/masterslave/${example}.tesh)
endforeach()

add_executable       (masterslave_arg       masterslave_arg.c)
target_link_libraries(masterslave_arg       simgrid)
add_executable       (masterslave_kill      masterslave_kill.c)
target_link_libraries(masterslave_kill      simgrid)

ADD_TESH_FACTORIES(msg-masterslave             "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/masterslave/masterslave_crosstraffic.tesh)
ADD_TESH_FACTORIES(msg-masterslave-kill        "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/masterslave/masterslave_kill.tesh)
ADD_TESH_FACTORIES(msg-masterslave-multicore   "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/masterslave/masterslave_multicore.tesh)
ADD_TESH_FACTORIES(msg-masterslave-cpu-ti      "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/masterslave --cd ${CMAKE_HOME_DIRECTORY}/examples/msg masterslave/masterslave_cpu_ti_crosstraffic.tesh)
ADD_TESH_FACTORIES(msg-masterslave-vivaldi     "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/masterslave/masterslave_vivaldi.tesh)
ADD_TESH_FACTORIES(msg-masterslave-no-crosstraffic "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/masterslave/masterslave.tesh)

set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_arg.c
                                   ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_kill.c                          PARENT_SCOPE)
set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/masterslave.tesh
                                   ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_crosstraffic.tesh
                                   ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_kill.tesh
                                   ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_cpu_ti.tesh
                                   ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_cpu_ti_crosstraffic.tesh
                                   ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_cas01_im.tesh
                                   ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_multicore.tesh
                                   ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_vivaldi.tesh                    PARENT_SCOPE)
set(xml_files   ${xml_files}       ${CMAKE_CURRENT_SOURCE_DIR}/deployment_masterslave_forwarder.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/deployment_masterslave_mailbox.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/deployment_masterslave_kill.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/deployment_masterslave_mailbox_multicore.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/deployment_masterslave_vivaldi.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/deployment_masterslave.xml                  PARENT_SCOPE)
