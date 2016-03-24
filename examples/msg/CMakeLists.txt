foreach(x dht-chord dht-pastry exception energy-consumption energy-onoff energy-pstate energy-ptask energy-vm 
          migration pmm priority properties suspend synchro token_ring)
  add_executable       (${x}     ${x}/${x}.c)
  target_link_libraries(${x}     simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
  set(examples_src ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)
  set(tesh_files   ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
endforeach()

foreach(file bourassa fafard ginette jupiter link3 link4 link5)
  set(txt_files ${txt_files} ${CMAKE_CURRENT_SOURCE_DIR}/${file}_state.trace)
endforeach()

set(txt_files    ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/README                                       PARENT_SCOPE)
set(bin_files    ${bin_files}     ${CMAKE_CURRENT_SOURCE_DIR}/dht-chord/generate.py                        PARENT_SCOPE)
set(examples_src ${examples_src}                                                                           PARENT_SCOPE)
set(tesh_files   ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/dht-chord/dht-chord_crosstraffic.tesh        PARENT_SCOPE)
set(xml_files    ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/dht-chord/chord.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dht-chord/chord10.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dht-pastry/pastry10.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/energy-onoff/platform_onoff.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/priority/priority_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/properties/properties_d.xml                  PARENT_SCOPE)

foreach(x dht-chord migration priority properties suspend synchro token_ring)
  ADD_TESH_FACTORIES(msg-${x} "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/${x} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/${x} ${x}.tesh)
endforeach()

foreach (example consumption onoff pstate vm)
  ADD_TESH_FACTORIES(msg-energy-${example} "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/energy-${example}/energy-${example}.tesh)
endforeach()

ADD_TESH_FACTORIES(msg-dht-chord-crosstraffic          "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/dht-chord --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/dht-chord dht-chord_crosstraffic.tesh)
ADD_TESH_FACTORIES(msg-dht-chord-crosstraffic-parallel "thread;ucontext;raw" --cfg contexts/nthreads:4 ${CONTEXTS_SYNCHRO} --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/dht-chord --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/dht-chord dht-chord_crosstraffic.tesh)
ADD_TESH_FACTORIES(msg-dht-chord-parallel              "thread;ucontext;raw" --cfg contexts/nthreads:4 ${CONTEXTS_SYNCHRO} --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/dht-chord --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/dht-chord dht-chord.tesh)
# ADD_TESH_FACTORIES(msg-dht-pastry "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg/dht-pastry --cd ${CMAKE_BINARY_DIR}/examples/msg/dht-pastry dht-pastry.tesh)
ADD_TESH_FACTORIES(msg-energy-pstate-ptask             "thread;ucontext;raw;boost" --cfg host/model:ptask_L07 --log xbt_cfg.threshold:critical --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/energy-pstate/energy-pstate.tesh)
ADD_TESH_FACTORIES(msg-energy-consumption-ptask        "thread;ucontext;raw;boost" --cfg host/model:ptask_L07 --log xbt_cfg.threshold:critical --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/energy-consumption/energy-consumption.tesh)
ADD_TESH_FACTORIES(msg-energy-ptask                    "thread;ucontext;raw" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/energy-ptask/energy-ptask.tesh)

ADD_TESH(msg-pmm  --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/pmm/pmm.tesh)
# This one is not usable:
# ADD_TESH_FACTORIES(msg-exception "thread;ucontext;raw" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/exception/exception.tesh)
