if(enable_smpi AND enable_smpi_MPICH3_testsuite)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

add_executable(cartcreates cartcreates.c)
#add_executable(cartmap1 cartmap1.c)
add_executable(cartshift1 cartshift1.c)
add_executable(cartsuball cartsuball.c)
add_executable(cartzero cartzero.c)
#  add_executable(dgraph_unwgt dgraph_unwgt.c)
#add_executable(dims1 dims1.c)
#add_executable(dims2 dims2.c)
#add_executable(dims3 dims3.c)
#add_executable(dims4 dims4.c)
#  add_executable(distgraph1 distgraph1.c)
#  add_executable(graphcr2 graphcr2.c)
#  add_executable(graphcr graphcr.c)
#  add_executable(graphmap1 graphmap1.c)
#add_executable(neighb_coll neighb_coll.c)
#  add_executable(topodup topodup.c)
#add_executable(topotest topotest.c)

target_link_libraries(cartcreates simgrid mtest_c)
# target_link_libraries(cartmap1 simgrid mtest_c)
target_link_libraries(cartshift1 simgrid mtest_c)
target_link_libraries(cartsuball simgrid mtest_c)
target_link_libraries(cartzero simgrid mtest_c)
#  target_link_libraries(dgraph_unwgt simgrid mtest_c)
#  target_link_libraries(dims1 simgrid mtest_c)
#  target_link_libraries(dims2 simgrid mtest_c)
#  target_link_libraries(dims3 simgrid mtest_c)
#  target_link_libraries(dims4 simgrid mtest_c)
#  target_link_libraries(distgraph1 simgrid mtest_c)
#  target_link_libraries(graphcr2 simgrid mtest_c)
#  target_link_libraries(graphcr simgrid mtest_c)
#  target_link_libraries(graphmap1 simgrid mtest_c)
#  target_link_libraries(neighb_coll simgrid mtest_c)
#  target_link_libraries(topodup simgrid mtest_c)
#  target_link_libraries(topotest simgrid mtest_c)
endif()

set(examples_src
 ${examples_src}
 ${CMAKE_CURRENT_SOURCE_DIR}/cartcreates.c
 ${CMAKE_CURRENT_SOURCE_DIR}/cartmap1.c
 ${CMAKE_CURRENT_SOURCE_DIR}/cartshift1.c
 ${CMAKE_CURRENT_SOURCE_DIR}/cartsuball.c
 ${CMAKE_CURRENT_SOURCE_DIR}/cartzero.c
 ${CMAKE_CURRENT_SOURCE_DIR}/dgraph_unwgt.c
 ${CMAKE_CURRENT_SOURCE_DIR}/dims1.c
 ${CMAKE_CURRENT_SOURCE_DIR}/dims2.c
 ${CMAKE_CURRENT_SOURCE_DIR}/dims3.c
 ${CMAKE_CURRENT_SOURCE_DIR}/dims4.c
 ${CMAKE_CURRENT_SOURCE_DIR}/distgraph1.c
 ${CMAKE_CURRENT_SOURCE_DIR}/graphcr2.c
 ${CMAKE_CURRENT_SOURCE_DIR}/graphcr.c
 ${CMAKE_CURRENT_SOURCE_DIR}/graphmap1.c
 ${CMAKE_CURRENT_SOURCE_DIR}/neighb_coll.c
 ${CMAKE_CURRENT_SOURCE_DIR}/testlist
 ${CMAKE_CURRENT_SOURCE_DIR}/topodup.c
 ${CMAKE_CURRENT_SOURCE_DIR}/topotest.c
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE)
