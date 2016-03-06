add_executable       (gtnets gtnets.c)
target_link_libraries(gtnets simgrid)

set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/gtnets-crosstraffic.tesh
                                   ${CMAKE_CURRENT_SOURCE_DIR}/gtnets-onelink-gtnets.tesh
                                   ${CMAKE_CURRENT_SOURCE_DIR}/tracing-gtnets-dogbone-gtnets.tesh
                                   ${CMAKE_CURRENT_SOURCE_DIR}/tracing-gtnets-onelink-lv08.tesh
                                   ${CMAKE_CURRENT_SOURCE_DIR}/gtnets-dogbone-gtnets.tesh
                                   ${CMAKE_CURRENT_SOURCE_DIR}/gtnets-onelink-lv08.tesh
                                   ${CMAKE_CURRENT_SOURCE_DIR}/tracing-gtnets-dogbone-lv08.tesh
                                   ${CMAKE_CURRENT_SOURCE_DIR}/gtnets-dogbone-lv08.tesh
                                   ${CMAKE_CURRENT_SOURCE_DIR}/tracing-gtnets-onelink-gtnets.tesh  PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/crosstraffic-d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/dogbone-d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/dogbone-p.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/onelink-d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/onelink-p.xml                       PARENT_SCOPE)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/gtnets.c                            PARENT_SCOPE)

IF(HAVE_GTNETS)
  ADD_TESH_FACTORIES(msg-gtnets-dogbone        "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/gtnets/gtnets-dogbone-gtnets.tesh)
  ADD_TESH_FACTORIES(msg-gtnets-onelink        "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/gtnets/gtnets-onelink-gtnets.tesh)
  ADD_TESH_FACTORIES(msg-gtnets-dogbone-lv08   "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/gtnets/gtnets-dogbone-lv08.tesh)
  ADD_TESH_FACTORIES(msg-gtnets-onelink-lv08   "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/gtnets/gtnets-onelink-lv08.tesh)
  ADD_TESH_FACTORIES(msg-gtnets-crosstraffic   "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/gtnets/gtnets-crosstraffic.tesh)
  ADD_TESH(msg-tracing-gtnets-dogbone        --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/gtnets/tracing-gtnets-dogbone-gtnets.tesh)
  ADD_TESH(msg-tracing-gtnets-onelink        --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/gtnets/tracing-gtnets-onelink-gtnets.tesh)
  ADD_TESH(msg-tracing-gtnets-dogbone-lv08   --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/gtnets/tracing-gtnets-dogbone-lv08.tesh)
  ADD_TESH(msg-tracing-gtnets-onelink-lv08   --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/gtnets/tracing-gtnets-onelink-lv08.tesh)
ENDIF()
