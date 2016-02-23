if(enable_smpi)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  endif()

  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  add_executable(compute compute.c)
  add_executable(compute2 compute2.c)
  add_executable(compute3 compute3.c)
  target_link_libraries(compute simgrid)
  target_link_libraries(compute2 simgrid)
  target_link_libraries(compute3 simgrid)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/compute.tesh
  PARENT_SCOPE)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/compute.c
  ${CMAKE_CURRENT_SOURCE_DIR}/compute2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/compute3.c
  PARENT_SCOPE)
