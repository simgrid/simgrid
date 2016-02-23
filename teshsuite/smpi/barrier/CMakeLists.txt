if(enable_smpi)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  endif()
  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  add_executable       (barrier_coll barrier.c)
  target_link_libraries(barrier_coll simgrid)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/barrier_coll.tesh
  PARENT_SCOPE)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/barrier.c
  PARENT_SCOPE)
