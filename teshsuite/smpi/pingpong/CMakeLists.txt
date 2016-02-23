if(enable_smpi)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  endif()

  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  add_executable(pingpong pingpong.c)
  add_executable(dsend dsend.c)
  target_link_libraries(pingpong simgrid)
  target_link_libraries(dsend simgrid)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/pt2pt.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/TI_output.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/broken_hostfiles.tesh
  PARENT_SCOPE)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/dsend.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pingpong.c
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/empty-hostfile.txt
  PARENT_SCOPE)
