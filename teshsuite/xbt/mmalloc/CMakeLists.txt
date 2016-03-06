if(HAVE_MMALLOC)
  add_executable       (mmalloc_test mmalloc_test.c)
  target_link_libraries(mmalloc_test simgrid)
  if(${ARCH_32_BITS})
    ADD_TESH(tesh-xbt-mmalloc-32 --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/xbt/mmalloc --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/xbt/mmalloc mmalloc_32.tesh)
  else()
    ADD_TESH(tesh-xbt-mmalloc-64 --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/xbt/mmalloc --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/xbt/mmalloc mmalloc_64.tesh)
  endif()
endif()

set(tesh_files     ${tesh_files}     ${CMAKE_CURRENT_SOURCE_DIR}/mmalloc_64.tesh
                                     ${CMAKE_CURRENT_SOURCE_DIR}/mmalloc_32.tesh  PARENT_SCOPE)
set(teshsuite_src  ${teshsuite_src}  ${CMAKE_CURRENT_SOURCE_DIR}/mmalloc_test.c   PARENT_SCOPE)
