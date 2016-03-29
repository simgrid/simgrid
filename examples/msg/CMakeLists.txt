foreach(x actions-mpi actions-storage async-wait async-waitall async-waitany dht-chord dht-pastry 
          exception energy-consumption energy-onoff energy-pstate energy-ptask energy-vm failures io-file io-file-unlink
          io-remote io-storage pmm priority process-kill process-migration process-suspend properties sendrecv
          set-maestro start_kill_time synchro token_ring 
          trace-categories trace-link-srcdst-user-variables trace-platform trace-simple trace-user-variables)
  add_executable       (${x}     ${x}/${x}.c)
  target_link_libraries(${x}     simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
  set(examples_src ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)
  set(tesh_files   ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
endforeach()

if(HAVE_NS3)
  add_executable       (ns3 ns3/ns3.c)
  target_link_libraries(ns3 simgrid)
  set_target_properties(ns3  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ns3)
endif()

set(txt_files    ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/README
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actions-mpi/actions-mpi.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actions-mpi/actions-mpi_split_p0.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actions-mpi/actions-mpi_split_p1.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actions-storage/actions-storage.txt          PARENT_SCOPE)
set(bin_files    ${bin_files}     ${CMAKE_CURRENT_SOURCE_DIR}/dht-chord/generate.py                        PARENT_SCOPE)
set(examples_src ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/ns3/ns3.c                                    PARENT_SCOPE)
set(tesh_files   ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/ns3/ns3.tesh                                 PARENT_SCOPE)
set(xml_files    ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/actions-mpi/actions-mpi_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actions-mpi/actions-mpi_split_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actions-storage/actions-storage_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/async-wait_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/async-wait2_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/async-wait3_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/async-wait4_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-waitall/async-waitall_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-waitany/async-waitany_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dht-chord/chord.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dht-chord/chord10.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dht-pastry/pastry10.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/energy-onoff/platform_onoff.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/io-remote/io-remote_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/ns3/3hosts_2links_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/ns3/3links-timer_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/ns3/3links_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/ns3/crosstraffic_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/ns3/dogbone_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/ns3/onelink_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/ns3/one_cluster_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/ns3/two_clusters_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/priority/priority_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/properties/properties_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv/sendrecv_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/set-maestro/set-maestro_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/start_kill_time/baseline_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/start_kill_time/kill_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/start_kill_time/start_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/start_kill_time/start_kill_d.xml             PARENT_SCOPE)

foreach(x actions-mpi actions-storage async-wait async-waitall async-waitany dht-chord failures io-file io-file-unlink 
        io-remote io-storage priority process-kill process-migration process-suspend properties sendrecv synchro 
        start_kill_time token_ring)
  ADD_TESH_FACTORIES(msg-${x} "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/${x} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/${x} ${x}.tesh)
endforeach()

foreach (example consumption onoff pstate vm)
  ADD_TESH_FACTORIES(msg-energy-${example} "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/energy-${example}/energy-${example}.tesh)
endforeach()

foreach (x categories link-srcdst-user-variables platform simple user-variables)
  ADD_TESH(msg-trace-${x} --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/trace-${x} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/trace-${x} trace-${x}.tesh)
endforeach()

ADD_TESH_FACTORIES(msg-dht-chord-parallel              "thread;ucontext;raw" --cfg contexts/nthreads:4 ${CONTEXTS_SYNCHRO} --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/dht-chord --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/dht-chord dht-chord.tesh)
ADD_TESH_FACTORIES(msg-energy-pstate-ptask             "thread;ucontext;raw;boost" --cfg host/model:ptask_L07 --log xbt_cfg.threshold:critical --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/energy-pstate/energy-pstate.tesh)
ADD_TESH_FACTORIES(msg-energy-consumption-ptask        "thread;ucontext;raw;boost" --cfg host/model:ptask_L07 --log xbt_cfg.threshold:critical --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/energy-consumption/energy-consumption.tesh)
ADD_TESH_FACTORIES(msg-energy-ptask                    "thread;ucontext;raw" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/energy-ptask/energy-ptask.tesh)
ADD_TESH_FACTORIES(msg-set-maestro                     "thread" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/set-maestro/set-maestro.tesh)

ADD_TESH(msg-pmm  --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/pmm/pmm.tesh)

if(HAVE_NS3)
  ADD_TESH_FACTORIES(msg-ns3 "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY} --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/ns3/ns3.tesh)
endif()

# These one are not usable:
# ADD_TESH_FACTORIES(msg-exception "thread;ucontext;raw" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/exception/exception.tesh)
# ADD_TESH_FACTORIES(msg-dht-pastry                    "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg/dht-pastry --cd ${CMAKE_BINARY_DIR}/examples/msg/dht-pastry dht-pastry.tesh)
