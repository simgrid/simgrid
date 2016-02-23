if(HAVE_MMALLOC)
  add_executable       (mmalloc_test mmalloc_test.c)
  target_link_libraries(mmalloc_test simgrid)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/mmalloc_64.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/mmalloc_32.tesh
  PARENT_SCOPE)
set(teshsuite_src
  ${teshsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/mmalloc_test.c
  PARENT_SCOPE)
