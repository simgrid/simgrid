configure_file("${CMAKE_HOME_DIRECTORY}/tools/tesh/tesh.py"
  "${CMAKE_BINARY_DIR}/bin/tesh" @ONLY IMMEDIATE)


foreach(x setenv set-output-ignore set-output-sort set-return set-timeout background basic bg-basic bg-set-signal
          catch-all-bg-output catch-return catch-signal catch-timeout catch-timeout-output catch-wrong-output cd color
          ignore-regexp IO-bigsize IO-broken-pipe IO-orders)
  set(tesh_files  ${tesh_files} ${CMAKE_CURRENT_SOURCE_DIR}/${x}.tesh)
  if(NOT enable_memcheck AND NOT WIN32)
    ADD_TESH(tesh-self-${x} --setenv bindir=${CMAKE_BINARY_DIR}/bin ${CMAKE_HOME_DIRECTORY}/tools/tesh/${x}.tesh)
  endif()
endforeach()

set(tesh_files ${tesh_files}                                                                               PARENT_SCOPE)
set(bin_files  ${bin_files}  ${CMAKE_CURRENT_SOURCE_DIR}/tesh.py ${CMAKE_CURRENT_SOURCE_DIR}/generate_tesh PARENT_SCOPE)
