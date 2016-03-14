if(HAVE_MC)
  add_executable       (dwarf-expression dwarf_expression.cpp)
  target_link_libraries(dwarf-expression simgrid)
endif()

set(tesh_files     ${tesh_files}     ${CMAKE_CURRENT_SOURCE_DIR}/dwarf_expression.tesh  PARENT_SCOPE)
set(testsuite_src  ${testsuite_src}  ${CMAKE_CURRENT_SOURCE_DIR}/dwarf_expression.cpp   PARENT_SCOPE)
