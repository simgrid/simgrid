
# Regular examples: with only one source and tested with all factories by default
#################################################################################
### Define the examples' specificities

set(_app-bittorrent_sources app-bittorrent/s4u-bittorrent.cpp app-bittorrent/s4u-bittorrent.hpp
                            app-bittorrent/s4u-peer.cpp app-bittorrent/s4u-peer.hpp
                            app-bittorrent/s4u-tracker.cpp app-bittorrent/s4u-tracker.hpp)
set(_dht-chord_sources    dht-chord/s4u-dht-chord.cpp dht-chord/s4u-dht-chord.hpp dht-chord/s4u-dht-chord-node.cpp)
set(_dht-kademlia_sources dht-kademlia/s4u-dht-kademlia.cpp dht-kademlia/s4u-dht-kademlia.hpp
                          dht-kademlia/node.cpp dht-kademlia/node.hpp
                          dht-kademlia/routing_table.cpp dht-kademlia/routing_table.hpp
                          dht-kademlia/answer.cpp dht-kademlia/answer.hpp dht-kademlia/message.hpp)

set(_actor-stacksize_factories "raw;boost;ucontext") # Threads ignore modifications of the stack size

# The maestro-set example only works for threads and when not using windows.
set(_maestro-set_factories "thread")
if(WIN32)
  set(_maestro-set_disable 1)
endif()

foreach (example mc-bugged1 mc-bugged2 mc-failing-assert mc-electric-fence)
  if(NOT SIMGRID_HAVE_MC)
    set(_${example}_disable 1)
  endif()
  set(_${example}_factories "ucontext;raw;boost")
endforeach()

if(SIMGRID_HAVE_MC)
   add_executable       (s4u-mc-bugged1-liveness  EXCLUDE_FROM_ALL mc-bugged1-liveness/s4u-mc-bugged1-liveness.cpp)
   target_link_libraries(s4u-mc-bugged1-liveness  simgrid)
   set_target_properties(s4u-mc-bugged1-liveness PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/mc-bugged1-liveness)
   add_dependencies(tests s4u-mc-bugged1-liveness)
 
  if(HAVE_C_STACK_CLEANER)
    add_executable       (s4u-mc-bugged1-liveness-cleaner-on  EXCLUDE_FROM_ALL s4u-mc-bugged1-liveness/s4u-mc-bugged1-liveness.cpp)
    target_link_libraries(s4u-mc-bugged1-liveness-cleaner-on  simgrid)
    set_target_properties(s4u-mc-bugged1-liveness-cleaner-on  PROPERTIES COMPILE_FLAGS "-DGARBAGE_STACK -fstack-cleaner")
    add_dependencies(tests s4u-mc-bugged1-liveness-cleaner-on)

    add_executable       (s4u-mc-bugged1-liveness-cleaner-off EXCLUDE_FROM_ALL s4u-mc-bugged1-liveness/s4u-mc-bugged1-liveness.cpp)
    target_link_libraries(s4u-mc-bugged1-liveness-cleaner-off simgrid)
    set_target_properties(s4u-mc-bugged1-liveness-cleaner-off PROPERTIES COMPILE_FLAGS "-DGARBAGE_STACK -fno-stack-cleaner")
    add_dependencies(tests s4u-mc-bugged1-liveness-cleaner-off)
  endif()
endif()

if(SIMGRID_HAVE_NS3)
  add_executable       (s4u-network-ns3 EXCLUDE_FROM_ALL network-ns3/s4u-network-ns3.cpp)
  target_link_libraries(s4u-network-ns3 simgrid)
  set_target_properties(s4u-network-ns3  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/network-ns3)
  add_dependencies(tests s4u-network-ns3)
  add_executable       (s4u-network-ns3-wifi EXCLUDE_FROM_ALL network-ns3-wifi/s4u-network-ns3-wifi.cpp)
  target_link_libraries(s4u-network-ns3-wifi simgrid)
  set_target_properties(s4u-network-ns3-wifi  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/network-ns3-wifi)
  add_dependencies(tests s4u-network-ns3-wifi)
endif()

# Deal with each example

foreach (example actor-create actor-daemon actor-exiting actor-join actor-kill
                 actor-lifetime actor-migrate actor-suspend actor-yield actor-stacksize
                 app-bittorrent app-chainsend app-token-ring
                 comm-pingpong comm-ready comm-suspend comm-wait comm-waitany comm-waitall comm-waituntil
                 comm-dependent comm-host2host
                 cloud-capping cloud-migration cloud-simple
                 dht-chord dht-kademlia
                 energy-exec energy-boot energy-link energy-vm energy-exec-ptask energy-wifi
                 engine-filtering
                 exec-async exec-basic exec-dvfs exec-ptask exec-remote exec-waitany exec-waitfor exec-dependent exec-unassigned
                 maestro-set
                 mc-bugged1 mc-bugged2 mc-electric-fence mc-failing-assert
		 network-wifi
                 io-async io-file-system io-file-remote io-disk-raw io-dependent
                 platform-failures platform-profile platform-properties
                 plugin-host-load plugin-link-load
                 replay-comm replay-io
                 routing-get-clusters
                 synchro-barrier synchro-condition-variable synchro-condition-variable-waituntil synchro-mutex synchro-semaphore)

  # Use default source file unless specified otherwise
  if(NOT DEFINED _${example}_sources)
    set(_${example}_sources ${example}/s4u-${example}.cpp)
  endif()

  if(NOT DEFINED _${example}_disable)
    add_executable       (s4u-${example} EXCLUDE_FROM_ALL ${_${example}_sources})
    add_dependencies     (tests s4u-${example})
    target_link_libraries(s4u-${example} simgrid)
    set_target_properties(s4u-${example} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${example})

    # Use default factories unless specified otherwise
    if(NOT DEFINED _${example}_factories)
      set(_${example}_factories "thread;ucontext;raw;boost")
    endif()
#    message("Factories of ${example}: ${_${example}_factories}")

    ADD_TESH_FACTORIES(s4u-${example} "${_${example}_factories}"
                                      --setenv bindir=${CMAKE_CURRENT_BINARY_DIR}/${example}
                                      --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
                                      --cd ${CMAKE_CURRENT_SOURCE_DIR}/${example}
                                      ${CMAKE_HOME_DIRECTORY}/examples/cpp/${example}/s4u-${example}.tesh)
  else()
    message(STATUS "Example ${example} disabled, thus not compiled.")
    unset(_${example}_disable)
  endif()

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${example}/s4u-${example}.tesh)
  foreach(file ${_${example}_sources})
    set(examples_src  ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${file})
  endforeach()

  unset(_${example}_factories)
  unset(_${example}_sources)
endforeach()


# Specific examples
###################

# MASTERWORKERS EXAMPLE
foreach(variant fun class) 
  add_executable       (s4u-app-masterworkers-${variant} EXCLUDE_FROM_ALL app-masterworkers/s4u-app-masterworkers-${variant}.cpp)
  target_link_libraries(s4u-app-masterworkers-${variant} simgrid)
  set_target_properties(s4u-app-masterworkers-${variant} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/app-masterworkers)
  add_dependencies(tests s4u-app-masterworkers-${variant})

  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/app-masterworkers/s4u-app-masterworkers-${variant}.cpp)
endforeach()
set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/app-masterworkers/s4u-app-masterworkers.tesh)

# Model-checking liveness
if(SIMGRID_HAVE_MC)
  IF(HAVE_UCONTEXT_CONTEXTS AND SIMGRID_PROCESSOR_x86_64) # liveness model-checking works only on 64bits (for now ...)
    ADD_TESH(s4u-mc-bugged1-liveness-ucontext     --setenv bindir=${CMAKE_CURRENT_BINARY_DIR}/mc-bugged1-liveness 
                                                  --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms 
                                                  --cd ${CMAKE_CURRENT_SOURCE_DIR}/mc-bugged1-liveness
                                                  ${CMAKE_HOME_DIRECTORY}/examples/cpp/mc-bugged1-liveness/s4u-mc-bugged1-liveness.tesh)

# This example hit the 5' timeout on CI, disable it for now
#    ADD_TESH(s4u-mc-bugged1-liveness-visited-ucontext --setenv bindir=${CMAKE_CURRENT_BINARY_DIR}/mc-bugged1-liveness 
#                                                      --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms 
#                                                      --cd ${CMAKE_CURRENT_SOURCE_DIR}/mc-bugged1-liveness
#                                                       ${CMAKE_HOME_DIRECTORY}/examples/cpp/mc-bugged1-liveness/s4u-mc-bugged1-liveness-visited.tesh)
    IF(HAVE_C_STACK_CLEANER)
      # This test checks if the stack cleaner is making a difference:
      ADD_TEST(s4u-mc-bugged1-liveness-stack-cleaner ${CMAKE_HOME_DIRECTORY}/examples/cpp/mc-bugged1-liveness/s4u-mc-bugged1-liveness-stack-cleaner
                                                     ${CMAKE_HOME_DIRECTORY}/examples/cpp/mc-bugged1-liveness/ 
                                                     ${CMAKE_CURRENT_BINARY_DIR}/mc-bugged1-liveness/)
    ENDIF()
  ENDIF()

#  if (enable_coverage)
#    SET_TESTS_PROPERTIES(mc-bugged1-liveness-visited-ucontext PROPERTIES RUN_SERIAL "TRUE")
#  endif()
ENDIF()

# The tests the parallel variant of of DHTs

if(CMAKE_SYSTEM_NAME STREQUAL "SunOS" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "i386" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
  # Thread-local storage (TLS) is needed for parallel execution, but it doesn't
  # play well with Ucontexts on 64bit SunOS (at least on x86_64).
  set(parallel-factories "thread;raw;boost")
else()
  set(parallel-factories "thread;ucontext;raw;boost")
endif()

foreach(example app-bittorrent app-masterworkers 
                dht-chord dht-kademlia
                )
  ADD_TESH_FACTORIES(s4u-${example}-parallel "${parallel-factories}" --cfg contexts/nthreads:4 ${CONTEXTS_SYNCHRO}
                                             --setenv bindir=${CMAKE_CURRENT_BINARY_DIR}/${example} 
                                             --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms 
                                             --cd ${CMAKE_CURRENT_SOURCE_DIR}/${example} 
                                             ${CMAKE_HOME_DIRECTORY}/examples/cpp/${example}/s4u-${example}.tesh)
endforeach()

# ns3-tests
if(SIMGRID_HAVE_NS3)
  ADD_TESH_FACTORIES(s4u-network-ns3 "thread;ucontext;raw;boost"
                                     --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/cpp
                                     --setenv bindir=${CMAKE_BINARY_DIR}/examples/cpp
                                     --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
                                     ${CMAKE_HOME_DIRECTORY}/examples/cpp/network-ns3/s4u-network-ns3.tesh)
  ADD_TESH_FACTORIES(s4u-network-ns3-wifi "thread;ucontext;raw;boost"
                                     --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/cpp
                                     --setenv bindir=${CMAKE_BINARY_DIR}/examples/cpp
                                     --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
                                     ${CMAKE_HOME_DIRECTORY}/examples/cpp/network-ns3-wifi/s4u-network-ns3-wifi.tesh)
endif()
# Examples not accepting factories
##################################

foreach (example trace-categories trace-masterworkers trace-platform trace-process-migration
                 trace-host-user-variables trace-link-user-variables trace-route-user-variables)
  add_executable       (s4u-${example} EXCLUDE_FROM_ALL ${example}/s4u-${example}.cpp)
  target_link_libraries(s4u-${example} simgrid)
  set_target_properties(s4u-${example} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${example})
  add_dependencies(tests s4u-${example})

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${example}/s4u-${example}.tesh)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/${example}/s4u-${example}.cpp)
  
  ADD_TESH(s4u-${example} --setenv bindir=${CMAKE_CURRENT_BINARY_DIR}/${example}
                          --setenv srcdir=${CMAKE_CURRENT_SOURCE_DIR}/${example}
                          --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms 
                          --cd ${CMAKE_CURRENT_BINARY_DIR}/${example}
                          ${CMAKE_HOME_DIRECTORY}/examples/cpp/${example}/s4u-${example}.tesh)
endforeach()

if (NOT enable_memcheck AND NOT WIN32)
  ADD_TESH(simix-breakpoint --setenv bindir=${CMAKE_CURRENT_BINARY_DIR}/comm-pingpong
                            --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
                            ${CMAKE_CURRENT_SOURCE_DIR}/comm-pingpong/simix-breakpoint.tesh)
endif()

if(enable_coverage AND SIMGRID_HAVE_MC)
  foreach (example mc-bugged1 mc-bugged2 mc-electric-fence mc-failing-assert)
    ADD_TEST(cover-${example} ${CMAKE_CURRENT_BINARY_DIR}/${example}/s4u-${example} ${CMAKE_HOME_DIRECTORY}/examples/platforms/model_checker_platform.xml)
  endforeach()
  ADD_TEST(cover-mc-bugged1-liveness ${CMAKE_CURRENT_BINARY_DIR}/mc-bugged1-liveness/s4u-mc-bugged1-liveness ${CMAKE_HOME_DIRECTORY}/examples/platforms/small_platform.xml 1 1001)
endif()

# Add all extra files to the archive
####################################
set(examples_src  ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/mc-bugged1-liveness/s4u-mc-bugged1-liveness.cpp
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/s4u-network-ns3.cpp              
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3-wifi/s4u-network-ns3-wifi.cpp              PARENT_SCOPE)
set(tesh_files    ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/comm-pingpong/simix-breakpoint.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/mc-bugged1-liveness/s4u-mc-bugged1-liveness.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/mc-bugged1-liveness/s4u-mc-bugged1-liveness-visited.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/s4u-network-ns3.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3-wifi/s4u-network-ns3-wifi.tesh             PARENT_SCOPE)
set(xml_files     ${xml_files}    ${CMAKE_CURRENT_SOURCE_DIR}/actor-create/s4u-actor-create_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actor-lifetime/s4u-actor-lifetime_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actor-yield/s4u-actor-yield_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/s4u-app-bittorrent_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/app-masterworkers/s4u-app-masterworkers_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/comm-suspend/s4u-comm-suspend_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/comm-wait/s4u-comm-wait_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/comm-waitany/s4u-comm-waitany_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/comm-waitall/s4u-comm-waitall_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/comm-ready/s4u-comm-ready_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/comm-waituntil/s4u-comm-waituntil_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dht-chord/s4u-dht-chord_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dht-kademlia/s4u-dht-kademlia_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/energy-boot/platform_boot.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/io-file-remote/s4u-io-file-remote_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/platform-properties/s4u-platform-properties_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/platform-failures/s4u-platform-failures_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/replay-comm/s4u-replay-comm-split_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/replay-comm/s4u-replay-comm_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/replay-io/s4u-replay-io_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/3hosts_2links_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/3links-timer_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/3links_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/crosstraffic_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/dogbone_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/onelink_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/one_cluster_d.xml                PARENT_SCOPE)
set(bin_files     ${bin_files}    ${CMAKE_CURRENT_SOURCE_DIR}/dht-kademlia/generate.py
                                  ${CMAKE_CURRENT_SOURCE_DIR}/mc-bugged1-liveness/s4u-mc-bugged1-liveness-stack-cleaner
                                  ${CMAKE_CURRENT_SOURCE_DIR}/mc-bugged1-liveness/promela_bugged1_liveness PARENT_SCOPE)
set(txt_files     ${txt_files}    ${CMAKE_CURRENT_SOURCE_DIR}/replay-comm/s4u-replay-comm-split-p0.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/replay-comm/s4u-replay-comm-split-p1.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/replay-comm/s4u-replay-comm.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/replay-io/s4u-replay-io.txt                  PARENT_SCOPE)

