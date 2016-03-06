add_executable       (token_ring ring_call.c)
target_link_libraries(token_ring simgrid)

ADD_TESH_FACTORIES(msg-token-ring "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY} --cd ${CMAKE_BINARY_DIR}/examples/msg/token_ring ${CMAKE_HOME_DIRECTORY}/examples/msg/token_ring/token_ring.tesh)

set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/token_ring.tesh  PARENT_SCOPE)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/ring_call.c      PARENT_SCOPE)
