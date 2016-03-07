foreach (example ms categories procmig trace_platform user_variables link_user_variables link_srcdst_user_variables)
  add_executable       (${example} ${example}.c)
  target_link_libraries(${example} simgrid)
  set(examples_src  ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${example}.c)
  set(tesh_files  ${tesh_files} ${CMAKE_CURRENT_SOURCE_DIR}/${example}.tesh)

  ADD_TESH(tracing-${example} --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg --cd ${CMAKE_HOME_DIRECTORY}/examples/msg tracing/${example}.tesh)
endforeach()

add_executable       (simple                     simple.c)
target_link_libraries(simple                     simgrid)

## Clean generated files
get_directory_property(extra_clean_files ADDITIONAL_MAKE_CLEAN_FILES)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${extra_clean_files};${CMAKE_CURRENT_BINARY_DIR}/ms.cat.plist;${CMAKE_CURRENT_BINARY_DIR}/ms.trace;${CMAKE_CURRENT_BINARY_DIR}/ms.uncat.plist;")

set(tesh_files    ${tesh_files}                                                    PARENT_SCOPE)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/simple.c            PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/deployment.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/procmig-deploy.xml  PARENT_SCOPE)
