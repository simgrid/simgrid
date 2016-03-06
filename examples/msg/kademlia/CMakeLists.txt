add_executable       (kademlia kademlia.c node.c routing_table.c task.c answer.c)
target_link_libraries(kademlia simgrid)

ADD_TESH_FACTORIES(msg-kademlia "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/kademlia --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/kademlia kademlia.tesh)
ADD_TESH(msg-kademlia-parallel  --cfg contexts/nthreads:4 --cfg contexts/factory:thread ${CONTEXTS_SYNCHRO} --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/kademlia --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/kademlia kademlia.tesh)

foreach (file answer kademlia node routing_table task)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/${file}.c  ${CMAKE_CURRENT_SOURCE_DIR}/${file}.h)
endforeach()

set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/common.h         PARENT_SCOPE)
set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/kademlia.tesh    PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/kademlia.xml     PARENT_SCOPE)
set(bin_files     ${bin_files}     ${CMAKE_CURRENT_SOURCE_DIR}/generate.py      PARENT_SCOPE)
