add_executable       (surf_usage  surf_usage.cpp)
target_link_libraries(surf_usage  simgrid)
add_executable       (surf_usage2 surf_usage2.cpp)
target_link_libraries(surf_usage2 simgrid )

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/surf_usage.tesh
  PARENT_SCOPE)
set(testsuite_src
  ${testsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/surf_usage.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/surf_usage2.cpp
  PARENT_SCOPE)
