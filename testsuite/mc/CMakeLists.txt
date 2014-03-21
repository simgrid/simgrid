cmake_minimum_required(VERSION 2.6)

if(HAVE_MC)
  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
  add_executable(dwarf dwarf.c)
  target_link_libraries(dwarf simgrid)

  add_executable(dwarf-expression dwarf_expression.c)
  target_link_libraries(dwarf-expression simgrid)
endif()

set(testsuite_src
  ${testsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/dwarf.c
  ${CMAKE_CURRENT_SOURCE_DIR}/dwarf_expression.c
  PARENT_SCOPE
  )
