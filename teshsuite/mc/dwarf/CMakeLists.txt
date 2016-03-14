if(HAVE_MC)
  add_executable       (dwarf dwarf.cpp)
  target_link_libraries(dwarf simgrid)
endif()

set(tesh_files     ${tesh_files}     ${CMAKE_CURRENT_SOURCE_DIR}/dwarf.tesh  PARENT_SCOPE)
set(testsuite_src  ${testsuite_src}  ${CMAKE_CURRENT_SOURCE_DIR}/dwarf.cpp   PARENT_SCOPE)
