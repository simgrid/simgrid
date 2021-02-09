if(enable_smpi)
  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  add_executable       (masterworker_mailbox_smpi EXCLUDE_FROM_ALL masterworker_mailbox_smpi.cpp)
  target_link_libraries(masterworker_mailbox_smpi simgrid)
  ADD_TESH_FACTORIES(smpi-s4u-masterworker "*" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/smpi/smpi_s4u_masterworker --cd ${CMAKE_BINARY_DIR}/examples/smpi/smpi_s4u_masterworker ${CMAKE_HOME_DIRECTORY}/examples/smpi/smpi_s4u_masterworker/s4u_smpi.tesh)
  add_dependencies(tests masterworker_mailbox_smpi)
endif()

set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/s4u_smpi.tesh                            PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/deployment_masterworker_mailbox_smpi.xml PARENT_SCOPE)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/masterworker_mailbox_smpi.cpp            PARENT_SCOPE)
