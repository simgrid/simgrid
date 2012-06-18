cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/_numerous_rpc_simulator.c
							${CMAKE_CURRENT_BINARY_DIR}/_numerous_rpc_client.c
							${CMAKE_CURRENT_BINARY_DIR}/_numerous_rpc_server.c
							PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(numerous_rpc_client    ${CMAKE_CURRENT_SOURCE_DIR}/numerous_rpc.c
                                      ${CMAKE_CURRENT_BINARY_DIR}/_numerous_rpc_client.c)
add_executable(numerous_rpc_server    ${CMAKE_CURRENT_SOURCE_DIR}/numerous_rpc.c
                                      ${CMAKE_CURRENT_BINARY_DIR}/_numerous_rpc_server.c)

add_custom_command(OUTPUT            ${CMAKE_CURRENT_BINARY_DIR}/_numerous_rpc_server.c
                                     ${CMAKE_CURRENT_BINARY_DIR}/_numerous_rpc_client.c
                   DEPENDS           gras_stub_generator ${CMAKE_CURRENT_SOURCE_DIR}/numerous_rpc.xml
                   COMMAND           ${CMAKE_BINARY_DIR}/bin/gras_stub_generator numerous_rpc ${CMAKE_CURRENT_SOURCE_DIR}/numerous_rpc.xml
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(numerous_rpc_client gras m pthread )
  target_link_libraries(numerous_rpc_server gras m pthread )
else(NOT WIN32)
  target_link_libraries(numerous_rpc_client gras)
  target_link_libraries(numerous_rpc_server gras)
endif(NOT WIN32)

set(tesh_files
  ${tesh_files}
  PARENT_SCOPE
)
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/numerous_rpc.xml
  PARENT_SCOPE
)
set(teshsuite_src
  ${teshsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/numerous_rpc.c
  PARENT_SCOPE
)
set(bin_files
  ${bin_files}
  PARENT_SCOPE
)
set(txt_files
  ${txt_files}
  PARENT_SCOPE
)

set(EXAMPLES_CMAKEFILES_TXT
  ${EXAMPLES_CMAKEFILES_TXT}
  ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
  PARENT_SCOPE
  )