cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
set(LIBRARY_OUTPUT_PATH "${CMAKE_HOME_DIRECTORY}/lib")

add_executable(ms         ${CMAKE_CURRENT_SOURCE_DIR}/ms.c)
add_executable(categories ${CMAKE_CURRENT_SOURCE_DIR}/categories.c)
add_executable(procmig    ${CMAKE_CURRENT_SOURCE_DIR}/procmig.c)
add_executable(simple     ${CMAKE_CURRENT_SOURCE_DIR}/simple.c)
add_executable(trace_platform     ${CMAKE_CURRENT_SOURCE_DIR}/trace_platform.c)
add_executable(user_variables     ${CMAKE_CURRENT_SOURCE_DIR}/user_variables.c)
add_executable(link_user_variables     ${CMAKE_CURRENT_SOURCE_DIR}/link_user_variables.c)
add_executable(link_srcdst_user_variables     ${CMAKE_CURRENT_SOURCE_DIR}/link_srcdst_user_variables.c)

### Add definitions for compile
target_link_libraries(ms simgrid m )
target_link_libraries(categories simgrid m )
target_link_libraries(procmig simgrid m )
target_link_libraries(simple simgrid m )
target_link_libraries(trace_platform simgrid m )
target_link_libraries(user_variables simgrid m )
target_link_libraries(link_user_variables simgrid m )
target_link_libraries(link_srcdst_user_variables simgrid m )

## Clean generated files
get_directory_property(extra_clean_files ADDITIONAL_MAKE_CLEAN_FILES)
set_directory_properties(
  PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
    "${extra_clean_files};${CMAKE_CURRENT_BINARY_DIR}/ms.cat.plist;${CMAKE_CURRENT_BINARY_DIR}/ms.trace;${CMAKE_CURRENT_BINARY_DIR}/ms.uncat.plist;"
 )

 set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_BINARY_DIR}/categories.tesh
  ${CMAKE_CURRENT_BINARY_DIR}/link_srcdst_user_variables.tesh
  ${CMAKE_CURRENT_BINARY_DIR}/link_user_variables.tesh
  ${CMAKE_CURRENT_BINARY_DIR}/ms.tesh
  ${CMAKE_CURRENT_BINARY_DIR}/procmig.tesh
  ${CMAKE_CURRENT_BINARY_DIR}/trace_platform.tesh
  ${CMAKE_CURRENT_BINARY_DIR}/user_variables.tesh
)
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_BINARY_DIR}/deployment.xml
  ${CMAKE_CURRENT_BINARY_DIR}/platform.xml
  ${CMAKE_CURRENT_BINARY_DIR}/procmig-deploy.xml
)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_BINARY_DIR}/categories.c
  ${CMAKE_CURRENT_BINARY_DIR}/link_srcdst_user_variables.c
  ${CMAKE_CURRENT_BINARY_DIR}/link_user_variables.c
  ${CMAKE_CURRENT_BINARY_DIR}/ms.c
  ${CMAKE_CURRENT_BINARY_DIR}/procmig.c
  ${CMAKE_CURRENT_BINARY_DIR}/simple.c
  ${CMAKE_CURRENT_BINARY_DIR}/trace_platform.c
  ${CMAKE_CURRENT_BINARY_DIR}/user_variables.c
)
set(bin_files
  ${bin_files}
)
set(txt_files
  ${txt_files}
)
 