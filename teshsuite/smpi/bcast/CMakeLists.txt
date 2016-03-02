if(enable_smpi)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  endif()
  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  add_executable       (bcast bcast.c)
  target_link_libraries(bcast simgrid)
  add_executable       (bcast_coll bcast_coll.c)
  target_link_libraries(bcast_coll simgrid)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/bcast.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/bcast_coll.tesh
  PARENT_SCOPE)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/bcast.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bcast_coll.c
  PARENT_SCOPE)
