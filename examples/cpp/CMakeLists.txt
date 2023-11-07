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

# The maestro-set example only works for threads
set(_maestro-set_factories "thread")

if(SIMGRID_HAVE_MC)
  # These tests timeout with threads, not sure why
  foreach(example mc-bugged1 mc-bugged2 mc-failing-assert mc-electric-fence)
    set(_${example}_factories "^thread") # Timeout
    add_dependencies(tests-mc s4u-${example})
  endforeach()

  if(enable_coverage)
    foreach (example mc-bugged1 mc-bugged2 mc-electric-fence mc-failing-assert)
      ADD_TEST(cover-${example} ${CMAKE_CURRENT_BINARY_DIR}/${example}/s4u-${example} ${CMAKE_HOME_DIRECTORY}/examples/platforms/model_checker_platform.xml)
    endforeach()
  endif()
else()
  foreach (example mc-bugged1 mc-bugged2  mc-centralized-mutex mc-failing-assert mc-electric-fence)
    set(_${example}_disable 1)
  endforeach()
endif()

# Hijack some regular tests to run them on top of the MC
foreach (example synchro-barrier synchro-mutex synchro-semaphore)
  set(tesh_files ${tesh_files} ${CMAKE_CURRENT_SOURCE_DIR}/${example}/s4u-mc-${example}.tesh)

  if (SIMGRID_HAVE_MC)
    ADD_TESH(s4u-mc-${example}
              --setenv bindir=${CMAKE_CURRENT_BINARY_DIR}/${example}
              --setenv libdir=${CMAKE_BINARY_DIR}/lib
              --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
              --setenv srcdir=${CMAKE_CURRENT_SOURCE_DIR}/${example}
              --cd ${CMAKE_CURRENT_SOURCE_DIR}/${example}
              ${CMAKE_HOME_DIRECTORY}/examples/cpp/${example}/s4u-mc-${example}.tesh)

    add_dependencies(tests-mc s4u-${example})
  endif()
endforeach()

if (SIMGRID_HAVE_MC)
  # Dependency on the regular tests
  foreach(example mc-centralized-mutex)
    add_dependencies(tests-mc s4u-${example})
  endforeach()
endif()

if(NOT HAVE_GRAPHVIZ)
  set(_dag-from-dot_disable 1)
  set(_dag-from-dot-simple_disable 1)
endif()

if (NOT SIMGRID_HAVE_JSON)
  set(_dag-from-json-simple_disable 1)
endif()

if(SIMGRID_HAVE_NS3)
  if(NS3_VERSION VERSION_GREATER_EQUAL 3.37)
    set(_network-ns3_teshfile         ${CMAKE_HOME_DIRECTORY}/examples/cpp/network-ns3/s4u-network-ns3-timed.tesh)
    set(tesh_files    ${tesh_files}   ${CMAKE_HOME_DIRECTORY}/examples/cpp/network-ns3/s4u-network-ns3-notime.tesh)
  else()
    set(_network-ns3_teshfile         ${CMAKE_HOME_DIRECTORY}/examples/cpp/network-ns3/s4u-network-ns3-notime.tesh)
    set(tesh_files    ${tesh_files}   ${CMAKE_HOME_DIRECTORY}/examples/cpp/network-ns3/s4u-network-ns3-timed.tesh)
  endif()

foreach (example network-ns3 network-ns3-wifi)
  add_dependencies(tests-ns3 s4u-${example})
endforeach()

else()
  # Even if ns3 is not found, we need to override the teshfile name and make sure that everything gets included in the archive
  set(_network-ns3_teshfile         ${CMAKE_HOME_DIRECTORY}/examples/cpp/network-ns3/s4u-network-ns3-notime.tesh)
  set(tesh_files    ${tesh_files}   ${CMAKE_HOME_DIRECTORY}/examples/cpp/network-ns3/s4u-network-ns3-timed.tesh)

  foreach (example network-ns3 network-ns3-wifi)
    set(_${example}_disable 1)
  endforeach()
endif()

# Deal with each example

foreach (example activityset-testany activityset-waitany activityset-waitall activityset-waitallfor
                 actor-create actor-daemon actor-exiting actor-join actor-kill
                 actor-lifetime actor-migrate actor-suspend actor-yield actor-stacksize
                 app-bittorrent app-chainsend app-token-ring
                 battery-chiller-solar battery-connector battery-degradation battery-simple battery-energy
                 chiller-simple
                 comm-pingpong comm-ready comm-suspend comm-wait comm-waituntil
                 comm-dependent comm-host2host comm-failure comm-throttling
                 cloud-capping cloud-migration cloud-simple
                 dag-comm dag-from-json-simple dag-from-dax-simple dag-from-dax dag-from-dot-simple dag-from-dot dag-failure dag-io dag-scheduling dag-simple dag-tuto
                 dht-chord dht-kademlia
                 energy-exec energy-boot energy-link energy-vm energy-exec-ptask energy-wifi
                 engine-filtering engine-run-partial
                 exec-async exec-basic exec-dvfs exec-remote exec-waitfor exec-dependent exec-unassigned
                 exec-ptask-multicore exec-ptask-multicore-latency exec-cpu-nonlinear exec-cpu-factors exec-failure exec-threads
                 maestro-set
                 mc-bugged1 mc-bugged2 mc-centralized-mutex mc-electric-fence mc-failing-assert
                 mess-wait
                 network-ns3 network-ns3-wifi network-wifi
                 io-async io-priority io-degradation io-file-system io-file-remote io-disk-raw io-dependent
                 task-dispatch task-io task-microservice task-parallelism task-simple task-storm task-switch-host task-variable-load
                 solar-panel-simple
                 platform-comm-serialize platform-failures platform-profile platform-properties
                 plugin-host-load plugin-jbod plugin-link-load plugin-prodcons
                 replay-comm replay-io
                 routing-get-clusters
                 synchro-barrier synchro-condition-variable synchro-condition-variable-waituntil synchro-mutex synchro-semaphore
                 clusters-multicpu network-factors network-nonlinear)

  # Use default source file unless specified otherwise
  if(NOT DEFINED _${example}_sources)
    set(_${example}_sources ${example}/s4u-${example}.cpp)
  endif()

  # Use default tesh file unless specified otherwise
  if(NOT DEFINED _${example}_teshfile)
    set(_${example}_teshfile ${CMAKE_HOME_DIRECTORY}/examples/cpp/${example}/s4u-${example}.tesh)
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
                                      ${_${example}_teshfile})
  else()
    message(STATUS "Example ${example} disabled, thus not compiled.")
    unset(_${example}_disable)
  endif()

  set(tesh_files    ${tesh_files}    ${_${example}_teshfile})
  foreach(file ${_${example}_sources})
    set(examples_src  ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${file})
  endforeach()

  unset(_${example}_factories)
  unset(_${example}_sources)
  unset(_${example}_teshfile)
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

# Test non-DPOR reductions on a given MC test
foreach(example mc-failing-assert)

  if(SIMGRID_HAVE_MC)
    ADD_TESH(s4u-${example}-nodpor    --setenv bindir=${CMAKE_CURRENT_BINARY_DIR}/${example}
                                      --setenv libdir=${CMAKE_BINARY_DIR}/lib
                                      --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
                                      --setenv srcdir=${CMAKE_CURRENT_SOURCE_DIR}/${example}
                                      --cd ${CMAKE_CURRENT_SOURCE_DIR}/${example}
                                      ${CMAKE_HOME_DIRECTORY}/examples/cpp/${example}/s4u-${example}-nodpor.tesh)
  endif()
  set(tesh_files    ${tesh_files}   ${CMAKE_HOME_DIRECTORY}/examples/cpp/${example}/s4u-${example}-nodpor.tesh)
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

if (NOT enable_memcheck)
  ADD_TESH(debug-breakpoint --setenv bindir=${CMAKE_CURRENT_BINARY_DIR}/comm-pingpong
                            --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
                            ${CMAKE_CURRENT_SOURCE_DIR}/comm-pingpong/debug-breakpoint.tesh)
endif()

# Add all extra files to the archive
####################################
set(examples_src  ${examples_src}    PARENT_SCOPE)
set(tesh_files    ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/comm-pingpong/debug-breakpoint.tesh  PARENT_SCOPE)
set(xml_files     ${xml_files}    ${CMAKE_CURRENT_SOURCE_DIR}/actor-create/s4u-actor-create_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actor-lifetime/s4u-actor-lifetime_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/s4u-app-bittorrent_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/app-masterworkers/s4u-app-masterworkers_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dag-scheduling/Montage_25.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dht-chord/s4u-dht-chord_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dht-kademlia/s4u-dht-kademlia_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/io-file-remote/s4u-io-file-remote_d.xml
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
set(bin_files     ${bin_files}    ${CMAKE_CURRENT_SOURCE_DIR}/battery-degradation/plot_battery_degradation.py
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dht-kademlia/generate.py  PARENT_SCOPE)
set(txt_files     ${txt_files}    ${CMAKE_CURRENT_SOURCE_DIR}/dag-from-dax/simple_dax_with_cycle.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dag-from-dax/smalldax.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dag-from-dax-simple/dag.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dag-from-dot/dag.dot
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dag-from-dot-simple/dag.dot
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dag-from-dot/dag_with_cycle.dot
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dag-from-json-simple/dag.json
                                  ${CMAKE_CURRENT_SOURCE_DIR}/replay-comm/s4u-replay-comm-split-p0.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/replay-comm/s4u-replay-comm-split-p1.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/replay-comm/s4u-replay-comm.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/replay-io/s4u-replay-io.txt                  PARENT_SCOPE)
