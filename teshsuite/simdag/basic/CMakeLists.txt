set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(basic0 basic0.c)
add_executable(basic1 basic1.c)
add_executable(basic2 basic2.c)
add_executable(basic3 basic3.c)
add_executable(basic4 basic4.c)
add_executable(basic5 basic5.c)
add_executable(basic6 basic6.c)

target_link_libraries(basic0 simgrid)
target_link_libraries(basic1 simgrid)
target_link_libraries(basic2 simgrid)
target_link_libraries(basic3 simgrid)
target_link_libraries(basic4 simgrid)
target_link_libraries(basic5 simgrid)
target_link_libraries(basic6 simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/basic0.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/basic1.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/basic2.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/basic3.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/basic4.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/basic5.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/basic6.tesh
  PARENT_SCOPE)
set(teshsuite_src
  ${teshsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/basic0.c
  ${CMAKE_CURRENT_SOURCE_DIR}/basic1.c
  ${CMAKE_CURRENT_SOURCE_DIR}/basic2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/basic3.c
  ${CMAKE_CURRENT_SOURCE_DIR}/basic4.c
  ${CMAKE_CURRENT_SOURCE_DIR}/basic5.c
  ${CMAKE_CURRENT_SOURCE_DIR}/basic6.c
  PARENT_SCOPE)
