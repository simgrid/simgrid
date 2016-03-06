add_executable       (chord chord.c)
target_link_libraries(chord simgrid)

ADD_TESH_FACTORIES(msg-chord                   "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/chord --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/chord chord_crosstraffic.tesh)
ADD_TESH_FACTORIES(msg-chord-no-crosstraffic   "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/chord --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/chord chord.tesh)
ADD_TESH_FACTORIES(msg-chord-parallel                 "thread;ucontext;raw" --cfg contexts/nthreads:4 ${CONTEXTS_SYNCHRO} --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/chord --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/chord chord_crosstraffic.tesh)
ADD_TESH_FACTORIES(msg-chord-no-crosstraffic-parallel "thread;ucontext;raw" --cfg contexts/nthreads:4 ${CONTEXTS_SYNCHRO} --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/chord --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/chord chord.tesh)

set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/chord.tesh
                                   ${CMAKE_CURRENT_SOURCE_DIR}/chord_crosstraffic.tesh  PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/chord.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/chord10.xml              PARENT_SCOPE)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/chord.c                  PARENT_SCOPE)
set(bin_files     ${bin_files}     ${CMAKE_CURRENT_SOURCE_DIR}/generate.py              PARENT_SCOPE)
