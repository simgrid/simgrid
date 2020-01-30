if(enable_smpi)
  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  add_executable       (masterslave_mailbox_smpi EXCLUDE_FROM_ALL masterslave_mailbox_smpi.cpp)
  target_link_libraries(masterslave_mailbox_smpi simgrid)
  ADD_TESH_FACTORIES(smpi-s4u-masterslave "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/smpi/smpi_s4u_masterslave --cd ${CMAKE_BINARY_DIR}/examples/smpi/smpi_s4u_masterslave ${CMAKE_HOME_DIRECTORY}/examples/smpi/smpi_s4u_masterslave/s4u_smpi.tesh)
  add_dependencies(tests masterslave_mailbox_smpi)
endif()

set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/s4u_smpi.tesh                            PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/deployment_masterslave_mailbox_smpi.xml  PARENT_SCOPE)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_mailbox_smpi.cpp             PARENT_SCOPE)
