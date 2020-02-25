# Regular examples: with only one source and tested with all factories
######################################################################

foreach(x
        actor-create actor-daemon actor-exiting actor-join actor-kill actor-migrate actor-suspend actor-yield
        app-pingpong app-token-ring 
        async-wait async-waitall async-waitany
        cloud-capping cloud-migration cloud-simple
        exec-dvfs
        energy-exec energy-exec-ptask energy-vm
        io-disk-raw io-file-remote
        plugin-hostload)
  add_executable       (${x}-c EXCLUDE_FROM_ALL ${x}/${x}.c)
  target_link_libraries(${x}-c simgrid)
  set_target_properties(${x}-c PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
  add_dependencies(tests ${x}-c)

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)
endforeach()

# Multi-files examples
######################
# Chainsend example

add_executable       (app-chainsend-c EXCLUDE_FROM_ALL app-chainsend/chainsend.c app-chainsend/broadcaster.c 
                      app-chainsend/peer.c)
target_link_libraries(app-chainsend-c simgrid)
set_target_properties(app-chainsend-c PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/app-chainsend)
add_dependencies(tests app-chainsend-c)

foreach (file chainsend broadcaster peer)
  set(teshsuite_src  ${teshsuite_src}  ${CMAKE_CURRENT_SOURCE_DIR}/app-chainsend/${file}.c)
endforeach()
set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/app-chainsend/chainsend.h)

# Add all extra files to the archive
####################################

set(teshsuite_src ${teshsuite_src}  PARENT_SCOPE)
set(tesh_files    ${tesh_files} ${CMAKE_CURRENT_SOURCE_DIR}/app-chainsend/app-chainsend.tesh
                                PARENT_SCOPE)

set(xml_files     ${xml_files} ${CMAKE_CURRENT_SOURCE_DIR}/actor-create/actor-create_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/actor-yield/actor-yield_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/app-chainsend/app-chainsend_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/app-pingpong/app-pingpong_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/async-wait_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/async-wait2_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/async-wait3_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/async-wait4_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/async-waitall/async-waitall_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/async-waitany/async-waitany_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/io-file-remote/io-file-remote_d.xml
                               PARENT_SCOPE)

foreach(x
        actor-create actor-daemon actor-exiting actor-join actor-kill actor-migrate actor-suspend actor-yield
        app-chainsend app-pingpong app-token-ring
        async-wait async-waitall async-waitany
        cloud-capping cloud-migration cloud-simple
        exec-dvfs
        energy-exec energy-exec-ptask energy-vm
        io-disk-raw io-file-remote
        plugin-hostload)
  ADD_TESH(c-${x} --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
                  --setenv bindir=${CMAKE_BINARY_DIR}/examples/c/${x}
                  --cd ${CMAKE_HOME_DIRECTORY}/examples/c/${x}
                  ${CMAKE_HOME_DIRECTORY}/examples/c/${x}/${x}.tesh)
endforeach()
