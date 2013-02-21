cmake_minimum_required(VERSION 2.6)

if(enable_smpi)

  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
  set(MPICH_FLAGS "-DHAVE_STDLIB_H=1 -DHAVE_UNISTD_H=1 -DHAVE_STRING_H=1 -DUSE_STDARG=1 -DHAVE_LONG_DOUBLE=1 -DHAVE_PROTOTYPES=1 -DHAVE_SIGNAL_H=1 -DHAVE_SIGACTION=1 -DHAVE_SLEEP=1 -DHAVE_SYSCONF=1")

  include_directories("${CMAKE_HOME_DIRECTORY}/include/smpi")
  
  add_executable(coll1 coll1.c test.c)
  add_executable(coll2 coll2.c test.c)
  add_executable(coll3 coll3.c test.c)
  add_executable(coll4 coll4.c test.c)
  add_executable(coll5 coll5.c test.c)
  add_executable(coll6 coll6.c test.c)
  add_executable(coll7 coll7.c test.c)
  add_executable(coll8 coll8.c test.c)
  add_executable(coll9 coll9.c test.c)
  add_executable(coll10 coll10.c test.c)
  add_executable(coll11 coll11.c test.c)
  add_executable(coll12 coll12.c test.c)
  add_executable(coll13 coll13.c)
  add_executable(allredmany allredmany.c)
  add_executable(bcastbug2 bcastbug2.c test.c) 
  add_executable(bcastbug bcastbug.c test.c)
  add_executable(bcastvec bcastvec.c test.c )
  add_executable(grouptest grouptest.c test.c)
  add_executable(redtst redtst.c test.c)
  add_executable(barrier barrier.c test.c)
  add_executable(bcast_mpich bcast.c test.c)
  add_executable(allred allred.c ../pt2pt/gcomm.c)
  add_executable(allred2 allred2.c ../pt2pt/gcomm.c)
  add_executable(scatterv scatterv.c)
  add_executable(scattern scattern.c)
  add_executable(redscat redscat.c)
  add_executable(alltoallv_mpich alltoallv.c)
  add_executable(scantst scantst.c test.c)
  add_executable(longuser longuser.c test.c)

  if(SMPI_F2C)
    add_executable(allredf allredf.f)
    add_executable(assocf assocf.f)
    add_executable(allgatherf allgatherf.f)
    add_executable(bcastlog bcastlog.f)
  endif()

  target_link_libraries(coll1  simgrid smpi )
  target_link_libraries(coll2 simgrid smpi )
  target_link_libraries(coll3 simgrid smpi )
  target_link_libraries(coll4 simgrid smpi )
  target_link_libraries(coll5 simgrid smpi )
  target_link_libraries(coll6 simgrid smpi )
  target_link_libraries(coll7 simgrid smpi )
  target_link_libraries(coll8 simgrid smpi )
  target_link_libraries(coll9 simgrid smpi )
  target_link_libraries(coll10 simgrid smpi )
  target_link_libraries(coll11 simgrid smpi )
  target_link_libraries(coll12  simgrid smpi )
  target_link_libraries(coll13 simgrid smpi )
  target_link_libraries(allredmany simgrid smpi )
  target_link_libraries(bcastbug simgrid smpi )
  target_link_libraries(bcastbug2 simgrid smpi )
  target_link_libraries(bcastvec simgrid smpi )
  target_link_libraries(grouptest simgrid smpi )
  target_link_libraries(redtst simgrid smpi )
  target_link_libraries(barrier simgrid smpi )
  target_link_libraries(bcast_mpich simgrid smpi )
  target_link_libraries(allred simgrid smpi )
  target_link_libraries(allred2 simgrid smpi )
  target_link_libraries(scatterv simgrid smpi )
  target_link_libraries(scattern simgrid smpi )
  target_link_libraries(redscat simgrid smpi )
  target_link_libraries(longuser simgrid smpi )
  target_link_libraries(alltoallv_mpich simgrid smpi )
  target_link_libraries(scantst simgrid smpi )
  if(SMPI_F2C)
    target_link_libraries(allredf simgrid smpi )
    target_link_libraries(assocf simgrid smpi )
    target_link_libraries(allgatherf simgrid smpi )
    target_link_libraries(bcastlog simgrid smpi )
  endif()

  set_target_properties(coll1 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(coll2 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(coll3 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(coll4 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(coll5 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(coll6 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(coll7 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(coll8 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(coll9 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(coll10 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(coll11 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(coll12 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(coll13 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(allredmany PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(bcastbug PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(bcastbug2 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(bcastvec PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(grouptest PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(redtst PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(barrier PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(bcast_mpich PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(allred PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(allred2 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(scatterv PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(scattern PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(redscat PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(longuser PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(alltoallv_mpich PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(scantst PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  if(SMPI_F2C)
    set_target_properties(allredf PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
    set_target_properties(assocf PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
    set_target_properties(allgatherf PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
    set_target_properties(bcastlog PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  endif()

endif()

set(tesh_files
  ${tesh_files}
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/coll1.c
  ${CMAKE_CURRENT_SOURCE_DIR}/coll2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/coll3.c
  ${CMAKE_CURRENT_SOURCE_DIR}/coll4.c
  ${CMAKE_CURRENT_SOURCE_DIR}/coll5.c
  ${CMAKE_CURRENT_SOURCE_DIR}/coll6.c
  ${CMAKE_CURRENT_SOURCE_DIR}/coll7.c
  ${CMAKE_CURRENT_SOURCE_DIR}/coll8.c
  ${CMAKE_CURRENT_SOURCE_DIR}/coll9.c
  ${CMAKE_CURRENT_SOURCE_DIR}/coll10.c
  ${CMAKE_CURRENT_SOURCE_DIR}/coll11.c
  ${CMAKE_CURRENT_SOURCE_DIR}/coll12.c
  ${CMAKE_CURRENT_SOURCE_DIR}/coll13.c
  ${CMAKE_CURRENT_SOURCE_DIR}/allredmany.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bcastbug.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bcastbug2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bcastvec.c
  ${CMAKE_CURRENT_SOURCE_DIR}/grouptest.c
  ${CMAKE_CURRENT_SOURCE_DIR}/redtst.c
  ${CMAKE_CURRENT_SOURCE_DIR}/barrier.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bcast.c
  ${CMAKE_CURRENT_SOURCE_DIR}/allred.c
  ${CMAKE_CURRENT_SOURCE_DIR}/allred2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/scatterv.c
  ${CMAKE_CURRENT_SOURCE_DIR}/scattern.c
  ${CMAKE_CURRENT_SOURCE_DIR}/redscat.c
  ${CMAKE_CURRENT_SOURCE_DIR}/longuser.c
  ${CMAKE_CURRENT_SOURCE_DIR}/alltoallv.c
  ${CMAKE_CURRENT_SOURCE_DIR}/scantst.c
  ${CMAKE_CURRENT_SOURCE_DIR}/test.c
  ${CMAKE_CURRENT_SOURCE_DIR}/test.h
  ${CMAKE_CURRENT_SOURCE_DIR}/allgatherf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/allredf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/allredf.std
  ${CMAKE_CURRENT_SOURCE_DIR}/assocf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/bcastlog.f

  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/runtests
  ${CMAKE_CURRENT_SOURCE_DIR}/allredmany.std
  ${CMAKE_CURRENT_SOURCE_DIR}/allred.std
  ${CMAKE_CURRENT_SOURCE_DIR}/allredf.std
  ${CMAKE_CURRENT_SOURCE_DIR}/coll13.std
  ${CMAKE_CURRENT_SOURCE_DIR}/scatterv.std
  PARENT_SCOPE
  )
