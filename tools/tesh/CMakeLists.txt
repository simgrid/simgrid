configure_file("${CMAKE_HOME_DIRECTORY}/tools/tesh/tesh.pl"
  "${CMAKE_BINARY_DIR}/bin/tesh" @ONLY IMMEDIATE)

    file(COPY        ${CMAKE_HOME_DIRECTORY}/tools/cmake/scripts/Diff.pm
         DESTINATION ${CMAKE_BINARY_DIR}/bin
         FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ)
    file(COPY        ${CMAKE_HOME_DIRECTORY}/tools/cmake/scripts/IPC
         DESTINATION ${CMAKE_BINARY_DIR}/bin)

foreach(x setenv set-output-ignore set-output-sort set-return set-timeout background basic bg-basic bg-set-signal
          catch-return catch-signal catch-timeout catch-wrong-output cd IO-bigsize IO-broken-pipe IO-orders)
  set(tesh_files  ${tesh_files} ${CMAKE_CURRENT_SOURCE_DIR}/${x}.tesh)
  if(NOT enable_memcheck AND NOT WIN32)
    ADD_TESH(tesh-self-${x} --setenv bindir=${CMAKE_BINARY_DIR}/bin ${CMAKE_HOME_DIRECTORY}/tools/tesh/${x}.tesh)
  endif()
endforeach()

set(tesh_files ${tesh_files}                                                                               PARENT_SCOPE)
set(bin_files  ${bin_files}  ${CMAKE_CURRENT_SOURCE_DIR}/tesh.pl ${CMAKE_CURRENT_SOURCE_DIR}/generate_tesh PARENT_SCOPE)
