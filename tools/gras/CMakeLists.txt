cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

add_executable(gras_stub_generator stub_generator.c unix_stub_generator.c windows_stub_generator.c)
add_executable(struct_diff struct_diff.c)

### Add definitions for compile
if(NOT WIN32)
    target_link_libraries(gras_stub_generator simgrid pthread m)
    target_link_libraries(struct_diff         simgrid pthread m)
else(NOT WIN32)
    target_link_libraries(gras_stub_generator simgrid)
    target_link_libraries(struct_diff         simgrid)
endif(NOT WIN32)

set(tesh_files
  ${tesh_files}
  PARENT_SCOPE
)
set(xml_files
  ${xml_files}
  PARENT_SCOPE
)
set(tools_src
  ${tools_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/windows_stub_generator.c
  ${CMAKE_CURRENT_SOURCE_DIR}/unix_stub_generator.c
  ${CMAKE_CURRENT_SOURCE_DIR}/struct_diff.c
  ${CMAKE_CURRENT_SOURCE_DIR}/stub_generator.c
  ${CMAKE_CURRENT_SOURCE_DIR}/gras_stub_generator.h
  PARENT_SCOPE
)
set(bin_files
  ${bin_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/s_smx_process_t
  ${CMAKE_CURRENT_SOURCE_DIR}/s_smx_simcall_t
  ${CMAKE_CURRENT_SOURCE_DIR}/stub_generator4borland.mak
  ${CMAKE_CURRENT_SOURCE_DIR}/stub_generator.bpf
  ${CMAKE_CURRENT_SOURCE_DIR}/stub_generator.bpr
  PARENT_SCOPE
)
set(txt_files
  ${txt_files}
  PARENT_SCOPE
)