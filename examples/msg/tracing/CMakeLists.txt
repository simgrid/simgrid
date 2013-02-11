cmake_minimum_required(VERSION 2.6)

if(HAVE_TRACING)
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
  target_link_libraries(ms simgrid )
  target_link_libraries(categories simgrid )
  target_link_libraries(procmig simgrid )
  target_link_libraries(simple simgrid )
  target_link_libraries(trace_platform simgrid )
  target_link_libraries(user_variables simgrid )
  target_link_libraries(link_user_variables simgrid )
  target_link_libraries(link_srcdst_user_variables simgrid )

  ## Clean generated files
  get_directory_property(extra_clean_files ADDITIONAL_MAKE_CLEAN_FILES)
  set_directory_properties(
    PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
    "${extra_clean_files};${CMAKE_CURRENT_BINARY_DIR}/ms.cat.plist;${CMAKE_CURRENT_BINARY_DIR}/ms.trace;${CMAKE_CURRENT_BINARY_DIR}/ms.uncat.plist;"
    )
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/categories.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/link_srcdst_user_variables.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/link_user_variables.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/ms.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/procmig.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/trace_platform.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/user_variables.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/platform.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/procmig-deploy.xml
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/categories.c
  ${CMAKE_CURRENT_SOURCE_DIR}/link_srcdst_user_variables.c
  ${CMAKE_CURRENT_SOURCE_DIR}/link_user_variables.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ms.c
  ${CMAKE_CURRENT_SOURCE_DIR}/procmig.c
  ${CMAKE_CURRENT_SOURCE_DIR}/simple.c
  ${CMAKE_CURRENT_SOURCE_DIR}/trace_platform.c
  ${CMAKE_CURRENT_SOURCE_DIR}/user_variables.c
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
