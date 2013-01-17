cmake_minimum_required(VERSION 2.6)

if(HAVE_GRAPHVIZ)
  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

  #add_executable(<name_of_target> <src list>)
  add_executable(dot_test dot_test.c)
  add_executable(ptg_test ptg_test.c)
  add_executable(simulate_dot simulate_dot.c)
  add_executable(dot_test2 dot_test2.c)

  if(NOT WIN32)
    target_link_libraries(dot_test simgrid pthread m) 	#target_link_libraries(<name_of_targe> <dependencies>)
    target_link_libraries(ptg_test simgrid pthread m)   #target_link_libraries(<name_of_targe> <dependencies>)
    target_link_libraries(simulate_dot simgrid pthread m) 	#target_link_libraries(<name_of_targe> <dependencies>)
    target_link_libraries(dot_test2 simgrid pthread m) 	#target_link_libraries(<name_of_targe> <dependencies>)

    get_directory_property(extra_clean_files ADDITIONAL_MAKE_CLEAN_FILES)
    set_directory_properties(
      PROPERTIES
      ADDITIONAL_MAKE_CLEAN_FILES "${extra_clean_files};${CMAKE_CURRENT_BINARY_DIR}/dot_test; ${CMAKE_CURRENT_BINARY_DIR}/simulate_dot;")

  else()
    target_link_libraries(dot_test simgrid) 	#target_link_libraries(<name_of_targe> <dependencies>)
    target_link_libraries(ptg_test simgrid)   #target_link_libraries(<name_of_targe> <dependencies>)
    target_link_libraries(simulate_dot simgrid) 	#target_link_libraries(<name_of_targe> <dependencies>)
    target_link_libraries(dot_test2 simgrid) 	#target_link_libraries(<name_of_targe> <dependencies>)
    set_directory_properties(
      PROPERTIES
      ADDITIONAL_MAKE_CLEAN_FILES "${extra_clean_files};${CMAKE_CURRENT_BINARY_DIR}/dot_test; ${CMAKE_CURRENT_BINARY_DIR}/simulate_dot;")
  endif()
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/test_simdag_dotload.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/dot_test.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ptg_test.c
  ${CMAKE_CURRENT_SOURCE_DIR}/dot_test2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/simulate_dot.c
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/dag_with_bad_schedule.dot
  ${CMAKE_CURRENT_SOURCE_DIR}/dag_with_cycle.dot
  ${CMAKE_CURRENT_SOURCE_DIR}/dag_with_good_schedule.dot
  ${CMAKE_CURRENT_SOURCE_DIR}/dag.dot
  ${CMAKE_CURRENT_SOURCE_DIR}/ptg.dot
  PARENT_SCOPE
  )
