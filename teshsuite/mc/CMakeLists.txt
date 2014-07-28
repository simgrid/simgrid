cmake_minimum_required(VERSION 2.6)

if(HAVE_MC)
  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
endif()

set(tesh_files
  ${tesh_files}
  PARENT_SCOPE
  )
set(testsuite_src
  ${testsuite_src}
  PARENT_SCOPE
  )
