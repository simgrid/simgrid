if(enable_smpi)
  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  add_executable       (replay_multiple replay_multiple.c)
  target_link_libraries(replay_multiple simgrid)
  IF(NOT HAVE_MC)
    ADD_TESH(smpi-replay-multiple --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple --cd ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple ${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/replay_multiple.tesh)
  ENDIF()
endif()

set(txt_files     ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/generate_multiple_deployment.sh
                                   ${CMAKE_CURRENT_SOURCE_DIR}/description_file
                                   ${CMAKE_CURRENT_SOURCE_DIR}/README
                                   ${CMAKE_CURRENT_SOURCE_DIR}/smpi_replay.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace0.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace1.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace2.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace3.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace4.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace5.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace6.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace7.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace8.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace9.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace10.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace11.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace12.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace13.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace14.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace15.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace16.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace17.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace18.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace19.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace20.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace21.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace22.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace23.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace24.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace25.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace26.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace27.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace28.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace29.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace30.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/ti_traces_32_1/ti_trace31.txt  PARENT_SCOPE)
set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/replay_multiple.tesh           PARENT_SCOPE)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/replay_multiple.c              PARENT_SCOPE)

