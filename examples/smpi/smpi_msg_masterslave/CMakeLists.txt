if(enable_smpi)
  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  add_executable       (masterslave_mailbox_smpi masterslave_mailbox_smpi.c)
  target_link_libraries(masterslave_mailbox_smpi simgrid)
  ADD_TESH_FACTORIES(smpi-msg-masterslave "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/smpi/smpi_msg_masterslave --cd ${CMAKE_BINARY_DIR}/examples/smpi/smpi_msg_masterslave ${CMAKE_HOME_DIRECTORY}/examples/smpi/smpi_msg_masterslave/msg_smpi.tesh)
endif()

set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/msg_smpi.tesh                            PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/deployment_masterslave_mailbox_smpi.xml  PARENT_SCOPE)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_mailbox_smpi.c               PARENT_SCOPE)
