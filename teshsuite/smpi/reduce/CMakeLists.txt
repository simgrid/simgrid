if(enable_smpi)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  endif()
  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  add_executable       (reduce              reduce.c)
  target_link_libraries(reduce              simgrid)
  add_executable       (reduce_coll         reduce_coll.c)
  target_link_libraries(reduce_coll         simgrid)
  add_executable       (reduce_scatter_coll reduce_scatter_coll.c)
  target_link_libraries(reduce_scatter_coll simgrid)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/reduce.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/reduce_coll.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/reduce_scatter_coll.tesh
  PARENT_SCOPE)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/reduce.c
  ${CMAKE_CURRENT_SOURCE_DIR}/reduce_coll.c
  ${CMAKE_CURRENT_SOURCE_DIR}/reduce_scatter_coll.c
  PARENT_SCOPE)
