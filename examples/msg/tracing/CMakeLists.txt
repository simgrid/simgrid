add_executable       (ms                         ms.c)
target_link_libraries(ms                         simgrid)
add_executable       (categories                 categories.c)
target_link_libraries(categories                 simgrid)
add_executable       (procmig                    procmig.c)
target_link_libraries(procmig                    simgrid)
add_executable       (simple                     simple.c)
target_link_libraries(simple                     simgrid)
add_executable       (trace_platform             trace_platform.c)
target_link_libraries(trace_platform             simgrid)
add_executable       (user_variables             user_variables.c)
target_link_libraries(user_variables             simgrid)
add_executable       (link_user_variables        link_user_variables.c)
target_link_libraries(link_user_variables        simgrid)
add_executable       (link_srcdst_user_variables link_srcdst_user_variables.c)
target_link_libraries(link_srcdst_user_variables simgrid)

## Clean generated files
get_directory_property(extra_clean_files ADDITIONAL_MAKE_CLEAN_FILES)
set_directory_properties(
  PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
  "${extra_clean_files};${CMAKE_CURRENT_BINARY_DIR}/ms.cat.plist;${CMAKE_CURRENT_BINARY_DIR}/ms.trace;${CMAKE_CURRENT_BINARY_DIR}/ms.uncat.plist;"
  )

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/categories.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/link_srcdst_user_variables.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/link_user_variables.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/ms.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/procmig.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/trace_platform.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/user_variables.tesh
  PARENT_SCOPE)
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/procmig-deploy.xml
  PARENT_SCOPE)
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
  PARENT_SCOPE)
