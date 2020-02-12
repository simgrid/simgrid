foreach(x
        actor-create actor-daemon actor-exiting actor-join actor-kill actor-migrate actor-suspend actor-yield
        app-pingpong app-token-ring 
        async-waitall async-waitany 
        energy-exec
        io-disk-raw)
  add_executable       (${x}-c EXCLUDE_FROM_ALL ${x}/${x}.c)
  target_link_libraries(${x}-c simgrid)
  set_target_properties(${x}-c PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
  add_dependencies(tests ${x}-c)

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)
endforeach()

set(teshsuite_src ${teshsuite_src}  PARENT_SCOPE)
set(tesh_files    ${tesh_files}     PARENT_SCOPE)

set(xml_files     ${xml_files} ${CMAKE_CURRENT_SOURCE_DIR}/actor-create/actor-create_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/actor-yield/actor-yield_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/app-pingpong/app-pingpong_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/async-waitall/async-waitall_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/async-waitany/async-waitany_d.xml
                               PARENT_SCOPE)

foreach(x
        actor-create actor-daemon actor-exiting actor-join actor-kill actor-migrate actor-suspend actor-yield
        app-pingpong app-token-ring
        async-waitall async-waitany
        energy-exec
        io-disk-raw)
  ADD_TESH(c-${x} --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
                  --setenv bindir=${CMAKE_BINARY_DIR}/examples/c/${x}
                  --cd ${CMAKE_HOME_DIRECTORY}/examples/c/${x}
                  ${CMAKE_HOME_DIRECTORY}/examples/c/${x}/${x}.tesh)
endforeach()
