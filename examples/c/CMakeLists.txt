# Regular examples: with only one source and tested with all factories
######################################################################

foreach(x
        actor-create actor-daemon actor-exiting actor-join actor-kill actor-lifetime actor-migrate actor-stacksize
        actor-suspend actor-yield
        app-masterworker app-pingpong app-token-ring 
        async-wait async-waitall async-waitany
        cloud-capping cloud-masterworker cloud-migration cloud-simple
        dht-pastry
        exec-async exec-basic exec-dvfs exec-remote exec-waitany
        energy-exec energy-exec-ptask energy-vm
        io-disk-raw io-file-remote io-file-system
        platform-failures platform-properties
        plugin-hostload
        synchro-semaphore)
  add_executable       (c-${x} EXCLUDE_FROM_ALL ${x}/${x}.c)
  target_link_libraries(c-${x} simgrid)
  set_target_properties(c-${x} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
  add_dependencies(tests c-${x})

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)
endforeach()

# Multi-files examples
######################
# bittorrent example
add_executable       (c-app-bittorrent EXCLUDE_FROM_ALL app-bittorrent/app-bittorrent.c app-bittorrent/bittorrent-peer.c app-bittorrent/tracker.c)
target_link_libraries(c-app-bittorrent simgrid)
set_target_properties(c-app-bittorrent PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/app-bittorrent)
add_dependencies(tests c-app-bittorrent)

foreach (file app-bittorrent bittorrent-peer tracker)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/${file}.c  ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/${file}.h)
endforeach()

# Chainsend example

add_executable       (c-app-chainsend EXCLUDE_FROM_ALL app-chainsend/chainsend.c app-chainsend/broadcaster.c 
                      app-chainsend/peer.c)
target_link_libraries(c-app-chainsend simgrid)
set_target_properties(c-app-chainsend PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/app-chainsend)
add_dependencies(tests c-app-chainsend)

foreach (file chainsend broadcaster peer)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/app-chainsend/${file}.c)
endforeach()
set(examples_src ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/app-chainsend/chainsend.h)

#DHT-Kademlia
add_executable       (c-dht-kademlia EXCLUDE_FROM_ALL dht-kademlia/dht-kademlia.c dht-kademlia/node.c dht-kademlia/routing_table.c dht-kademlia/message.c dht-kademlia/answer.c)
target_link_libraries(c-dht-kademlia simgrid)
set_target_properties(c-dht-kademlia PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dht-kademlia)
add_dependencies(tests c-dht-kademlia)

foreach (file answer node routing_table message)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/dht-kademlia/${file}.c  ${CMAKE_CURRENT_SOURCE_DIR}/dht-kademlia/${file}.h)
endforeach()

# Add all extra files to the archive
####################################

set(tesh_files    ${tesh_files} ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/app-bittorrent.tesh
                                ${CMAKE_CURRENT_SOURCE_DIR}/app-chainsend/app-chainsend.tesh
                                ${CMAKE_CURRENT_SOURCE_DIR}/app-masterworker/app-masterworker-multicore.tesh
                                ${CMAKE_CURRENT_SOURCE_DIR}/app-masterworker/app-masterworker-vivaldi.tesh
                                ${CMAKE_CURRENT_SOURCE_DIR}/dht-kademlia/dht-kademlia.tesh
                                PARENT_SCOPE)
set(bin_files     ${bin_files} ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/generate.py
                               ${CMAKE_CURRENT_SOURCE_DIR}/dht-kademlia/generate.py
                               ${CMAKE_CURRENT_SOURCE_DIR}/dht-pastry/generate.py                          PARENT_SCOPE)
set(examples_src  ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/dht-kademlia/common.h
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dht-kademlia/dht-kademlia.c                  PARENT_SCOPE)
set(xml_files     ${xml_files} ${CMAKE_CURRENT_SOURCE_DIR}/actor-create/actor-create_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/actor-lifetime/actor-lifetime_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/actor-yield/actor-yield_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/app-bittorrent_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/app-chainsend/app-chainsend_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/app-masterworker/app-masterworker_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/app-masterworker/app-masterworker-multicore_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/app-masterworker/app-masterworker-vivaldi_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/app-pingpong/app-pingpong_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/async-wait_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/async-wait2_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/async-wait3_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/async-wait4_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/async-waitall/async-waitall_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/async-waitany/async-waitany_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/dht-kademlia/dht-kademlia_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/dht-pastry/dht-pastry_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/io-file-remote/io-file-remote_d.xml
                               ${CMAKE_CURRENT_SOURCE_DIR}/platform-properties/platform-properties_d.xml
                               PARENT_SCOPE)

foreach(x
        actor-create actor-daemon actor-exiting actor-join actor-kill actor-lifetime actor-migrate actor-stacksize
        actor-suspend actor-yield
        app-bittorrent app-chainsend app-masterworker app-pingpong app-token-ring
        async-wait async-waitall async-waitany
        cloud-capping  cloud-masterworker cloud-migration cloud-simple
        dht-kademlia dht-pastry
        exec-async exec-basic exec-dvfs exec-remote exec-waitany
        energy-exec energy-exec-ptask energy-vm
        io-disk-raw io-file-remote io-file-system
        platform-failures platform-properties
        plugin-hostload
        synchro-semaphore)
  ADD_TESH(c-${x} --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
                  --setenv bindir=${CMAKE_BINARY_DIR}/examples/c/${x}
                  --cd ${CMAKE_HOME_DIRECTORY}/examples/c/${x}
                  ${CMAKE_HOME_DIRECTORY}/examples/c/${x}/${x}.tesh)
endforeach()

ADD_TESH_FACTORIES(app-masterworker-multicore   "thread;ucontext;raw;boost" 
                                                --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/c/app-masterworker
                                                --setenv bindir=${CMAKE_BINARY_DIR}/examples/c/app-masterworker
                                                --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
                                               ${CMAKE_HOME_DIRECTORY}/examples/c/app-masterworker/app-masterworker-multicore.tesh)
ADD_TESH_FACTORIES(app-masterworker-vivaldi     "thread;ucontext;raw;boost" 
                                                --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/c/app-masterworker
                                                --setenv bindir=${CMAKE_BINARY_DIR}/examples/c/app-masterworker
                                                --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
                                                ${CMAKE_HOME_DIRECTORY}/examples/c/app-masterworker/app-masterworker-vivaldi.tesh)

if(CMAKE_SYSTEM_NAME STREQUAL "SunOS" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "i386" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    # Thread-local storage (TLS) is needed for parallel execution, but it doesn't
    # play well with Ucontexts on 64bit SunOS (at least on x86_64).
    set(parallel-factories "thread;raw;boost")
else()
    set(parallel-factories "thread;ucontext;raw;boost")
endif()

ADD_TESH_FACTORIES(c-app-bittorrent-parallel "raw" --cfg contexts/nthreads:4 ${CONTEXTS_SYNCHRO} 
                                             --setenv bindir=${CMAKE_BINARY_DIR}/examples/c/app-bittorrent 
                                             --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms 
                                             --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms 
                                             --cd ${CMAKE_HOME_DIRECTORY}/examples/c/app-bittorrent app-bittorrent.tesh)

ADD_TESH_FACTORIES(c-dht-kademlia-parallel "${parallel-factories}" --cfg contexts/nthreads:4 ${CONTEXTS_SYNCHRO}
                                           --setenv bindir=${CMAKE_BINARY_DIR}/examples/c/dht-kademlia
                                           --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/c/dht-kademlia
                                           --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
                                           ${CMAKE_HOME_DIRECTORY}/examples/c/dht-kademlia/dht-kademlia.tesh)
