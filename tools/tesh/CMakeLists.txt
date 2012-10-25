cmake_minimum_required(VERSION 2.6)

if(WIN32)
  add_custom_target(tesh ALL
    DEPENDS ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/Scripts/tesh.pl
    COMMENT "Install ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/Scripts/tesh.pl"
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/Scripts/tesh.pl ${CMAKE_BINARY_DIR}/bin/tesh
    )
else()
  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

  add_executable(tesh tesh.c run_context.c signal.c)

  ### Add definitions for compile
  target_link_libraries(tesh gras m pthread)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/setenv.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/set-ignore-output.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/set-return.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/set-signal.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/set-timeout.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/background.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/basic2.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/basic.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/bg-basic.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/bg-set-signal.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/catch-return.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/catch-signal.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/catch-timeout.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/catch-wrong-output.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/cd.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/IO-bigsize.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/IO-broken-pipe.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/IO-orders.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  PARENT_SCOPE
  )
set(tools_src
  ${tools_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/run_context.c
  ${CMAKE_CURRENT_SOURCE_DIR}/run_context.h
  ${CMAKE_CURRENT_SOURCE_DIR}/signal.c
  ${CMAKE_CURRENT_SOURCE_DIR}/tesh.c
  ${CMAKE_CURRENT_SOURCE_DIR}/tesh.h
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/colorize.pl
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/README
  PARENT_SCOPE
  )
