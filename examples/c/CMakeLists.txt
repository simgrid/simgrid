foreach(x async-waitany)
  add_executable       (${x} EXCLUDE_FROM_ALL ${x}/${x}.c)
  target_link_libraries(${x} simgrid)
  set_target_properties(${x} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
  add_dependencies(tests ${x})

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)
endforeach()

set(teshsuite_src ${teshsuite_src}  PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/async-waitany/async-waitany_d.xml    PARENT_SCOPE)

foreach(x async-waitany)
  ADD_TESH(c-${x} --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
                  --setenv bindir=${CMAKE_BINARY_DIR}/examples/c/${x}
                  --cd ${CMAKE_HOME_DIRECTORY}/examples/c/${x}
                  ${CMAKE_HOME_DIRECTORY}/examples/c/${x}/${x}.tesh)
endforeach()
