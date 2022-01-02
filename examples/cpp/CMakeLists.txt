
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

set(_actor-stacksize_factories "^thread") # Threads ignore modifications of the stack size

# The maestro-set example only works for threads and when not using windows.
set(_maestro-set_factories "thread")
if(WIN32)
  set(_maestro-set_disable 1)
endif()

if(SIMGRID_HAVE_MC)
  # These tests timeout with threads, maybe because of dwarf parsing? not sure
  foreach(example mc-bugged1 mc-bugged2 mc-failing-assert mc-electric-fence)
     set(_${example}_factories "^thread") # Timeout
     add_dependencies(tests-mc s4u-${example})
  endforeach()

  # Make all MC tests buildable together
  #foreach(example ) # no test to be build in any case
  #endforeach()
  

  if(HAVE_C_STACK_CLEANER)
    add_executable       (s4u-mc-bugged1-liveness-cleaner-on  EXCLUDE_FROM_ALL s4u-mc-bugged1-liveness/s4u-mc-bugged1-liveness.cpp)
    target_link_libraries(s4u-mc-bugged1-liveness-cleaner-on  simgrid)
    set_target_properties(s4u-mc-bugged1-liveness-cleaner-on  PROPERTIES COMPILE_FLAGS "-DGARBAGE_STACK -fstack-cleaner")
    add_dependencies(tests-mc s4u-mc-bugged1-liveness-cleaner-on)

    add_executable       (s4u-mc-bugged1-liveness-cleaner-off EXCLUDE_FROM_ALL s4u-mc-bugged1-liveness/s4u-mc-bugged1-liveness.cpp)
    target_link_libraries(s4u-mc-bugged1-liveness-cleaner-off simgrid)
    set_target_properties(s4u-mc-bugged1-liveness-cleaner-off PROPERTIES COMPILE_FLAGS "-DGARBAGE_STACK -fno-stack-cleaner")
    add_dependencies(tests-mc s4u-mc-bugged1-liveness-cleaner-off)
  endif()

  # Model-checking liveness
  if(HAVE_UCONTEXT_CONTEXTS AND SIMGRID_PROCESSOR_x86_64) 
    # liveness model-checking works only on 64bits (for now ...)
    set(_mc-bugged1-liveness_factories "ucontext") # Timeout
    add_dependencies(tests-mc s4u-mc-bugged1-liveness)
  else()
    set(_mc-bugged1-liveness_disable 1)
  endif()
  
  # This example hit the 5' timeout on CI, disable it for now
  #    ADD_TESH(s4u-mc-bugged1-liveness-visited-ucontext --setenv bindir=${CMAKE_CURRENT_BINARY_DIR}/mc-bugged1-liveness
  #                                                      --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
  #                                                      --cd ${CMAKE_CURRENT_SOURCE_DIR}/mc-bugged1-liveness
  #                                                       ${CMAKE_HOME_DIRECTORY}/examples/cpp/mc-bugged1-liveness/s4u-mc-bugged1-liveness-visited.tesh)
  IF(HAVE_C_STACK_CLEANER)
    add_dependencies(tests-mc s4u-mc-bugged1-liveness-stack-cleaner)
    # This test checks if the stack cleaner is making a difference:
    ADD_TEST(s4u-mc-bugged1-liveness-stack-cleaner ${CMAKE_HOME_DIRECTORY}/examples/cpp/mc-bugged1-liveness/s4u-mc-bugged1-liveness-stack-cleaner
                                                   ${CMAKE_HOME_DIRECTORY}/examples/cpp/mc-bugged1-liveness/
                                                   ${CMAKE_CURRENT_BINARY_DIR}/mc-bugged1-liveness/)
  ENDIF()
  
  if(enable_coverage)
    foreach (example mc-bugged1 mc-bugged2 mc-electric-fence mc-failing-assert)
      ADD_TEST(cover-${example} ${CMAKE_CURRENT_BINARY_DIR}/${example}/s4u-${example} ${CMAKE_HOME_DIRECTORY}/examples/platforms/model_checker_platform.xml)
    endforeach()
    ADD_TEST(cover-mc-bugged1-liveness ${CMAKE_CURRENT_BINARY_DIR}/mc-bugged1-liveness/s4u-mc-bugged1-liveness ${CMAKE_HOME_DIRECTORY}/examples/platforms/small_platform.xml 1 1001)
  endif()

else()
  foreach (example mc-bugged1 mc-bugged2 mc-failing-assert mc-electric-fence 
                   mc-bugged1-liveness)
    set(_${example}_disable 1)
  endforeach()
endif()

if(NOT HAVE_GRAPHVIZ)
  set(_dag-from-dot_disable 1)
endif()

if(NOT SIMGRID_HAVE_NS3)
  foreach (example network-ns3 network-ns3-wifi)
    set(_${example}_disable 1)
  endforeach()
endif()

# Deal with each example

foreach (example actor-create actor-daemon actor-exiting actor-join actor-kill
                 actor-lifetime actor-migrate actor-suspend actor-yield actor-stacksize
                 app-bittorrent app-chainsend app-token-ring
                 comm-pingpong comm-ready comm-serialize comm-suspend comm-wait comm-waitany comm-waitall comm-waituntil
                 comm-dependent comm-host2host comm-failure comm-throttling
                 cloud-capping cloud-migration cloud-simple
                 dag-comm dag-from-dax dag-from-dot dag-failure dag-io dag-scheduling dag-simple
                 dht-chord dht-kademlia
                 energy-exec energy-boot energy-link energy-vm energy-exec-ptask energy-wifi
                 engine-filtering engine-run-partial
                 exec-async exec-basic exec-dvfs exec-remote exec-waitany exec-waitfor exec-dependent exec-unassigned
                 exec-ptask-multicore exec-cpu-nonlinear exec-cpu-factors exec-failure
                 maestro-set
                 mc-bugged1 mc-bugged1-liveness mc-bugged2 mc-electric-fence mc-failing-assert
                 network-ns3 network-ns3-wifi network-wifi
                 io-async io-priority io-degradation io-file-system io-file-remote io-disk-raw io-dependent
                 platform-failures platform-profile platform-properties
                 plugin-host-load plugin-link-load plugin-prodcons
                 replay-comm replay-io
                 routing-get-clusters
                 synchro-barrier synchro-condition-variable synchro-condition-variable-waituntil synchro-mutex synchro-semaphore
                 clusters-multicpu network-factors network-nonlinear)

  # Use default source file unless specified otherwise
  if(NOT DEFINED _${example}_sources)
    set(_${example}_sources ${example}/s4u-${example}.cpp)
  endif()

  if(NOT DEFINED _${example}_disable)
    add_executable       (s4u-${example} EXCLUDE_FROM_ALL ${_${example}_sources})
    add_dependencies     (tests s4u-${example})
    add_dependencies     (s4u-${example} platf_cpp)
    target_link_libraries(s4u-${example} simgrid)
    set_target_properties(s4u-${example} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${example})

    # Use default factories unless specified otherwise
    if(NOT DEFINED _${example}_factories)
      set(_${example}_factories "*")
    endif()
#    message("Factories of ${example}: ${_${example}_factories}")

    ADD_TESH_FACTORIES(s4u-${example} "${_${example}_factories}"
                                      --setenv bindir=${CMAKE_CURRENT_BINARY_DIR}/${example}
                                      --setenv libdir=${CMAKE_BINARY_DIR}/lib
                                      --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
                                      --setenv srcdir=${CMAKE_CURRENT_SOURCE_DIR}/${example}
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

# The tests the parallel variant of of DHTs

if(CMAKE_SYSTEM_NAME STREQUAL "SunOS" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "i386" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
  # Thread-local storage (TLS) is needed for parallel execution, but it doesn't
  # play well with Ucontexts on 64bit SunOS (at least on x86_64).
  set(parallel-factories "^ucontext")
else()
  set(parallel-factories "*")
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

# Examples not accepting factories
##################################

foreach (example exec-ptask trace-categories trace-masterworkers trace-platform trace-process-migration
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

# Add all extra files to the archive
####################################
set(examples_src  ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/mc-bugged1-liveness/s4u-mc-bugged1-liveness.cpp        PARENT_SCOPE)
set(tesh_files    ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/comm-pingpong/simix-breakpoint.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/mc-bugged1-liveness/s4u-mc-bugged1-liveness.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/mc-bugged1-liveness/s4u-mc-bugged1-liveness-visited.tesh  PARENT_SCOPE)
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
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dag-scheduling/Montage_25.xml
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
set(txt_files     ${txt_files}    ${CMAKE_CURRENT_SOURCE_DIR}/dag-from-dax/simple_dax_with_cycle.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dag-from-dax/smalldax.xml     
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dag-from-dot/dag.dot
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dag-from-dot/dag_with_cycle.dot
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dag-scheduling/expected_output.jed
                                  ${CMAKE_CURRENT_SOURCE_DIR}/replay-comm/s4u-replay-comm-split-p0.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/replay-comm/s4u-replay-comm-split-p1.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/replay-comm/s4u-replay-comm.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/replay-io/s4u-replay-io.txt                  PARENT_SCOPE)
