configure_file("${CMAKE_HOME_DIRECTORY}/tools/tesh/tesh.pl"
  "${CMAKE_BINARY_DIR}/bin/tesh" @ONLY IMMEDIATE)

    file(COPY        ${CMAKE_HOME_DIRECTORY}/tools/cmake/scripts/Diff.pm
         DESTINATION ${CMAKE_BINARY_DIR}/bin
         FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
                          GROUP_EXECUTE GROUP_READ)
    file(COPY        ${CMAKE_HOME_DIRECTORY}/tools/cmake/scripts/IPC
         DESTINATION ${CMAKE_BINARY_DIR}/bin)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/setenv.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/set-output-ignore.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/set-output-sort.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/set-output-sort-1.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/set-return.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/set-signal.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/set-timeout.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/background.tesh
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
set(bin_files
  ${bin_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/tesh.pl
  ${CMAKE_CURRENT_SOURCE_DIR}/colorize.pl
  ${CMAKE_CURRENT_SOURCE_DIR}/generate_tesh
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/README
  ${CMAKE_CURRENT_SOURCE_DIR}/tesh.1
  PARENT_SCOPE
  )
