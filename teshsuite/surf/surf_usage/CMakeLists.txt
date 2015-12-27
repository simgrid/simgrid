set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(surf_usage ${CMAKE_HOME_DIRECTORY}/teshsuite/surf/surf_usage/surf_usage.cpp)
add_executable(surf_usage2 ${CMAKE_HOME_DIRECTORY}/teshsuite/surf/surf_usage/surf_usage2.cpp)

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(surf_usage simgrid m )
  target_link_libraries(surf_usage2 simgrid m )
else()
  target_link_libraries(surf_usage simgrid )
  target_link_libraries(surf_usage2 simgrid )
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/surf_usage.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  PARENT_SCOPE
  )
set(testsuite_src
  ${testsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/surf_usage.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/surf_usage2.cpp
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  PARENT_SCOPE
  )
