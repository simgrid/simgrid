if(enable_smpi AND enable_smpi_MPICH3_testsuite)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

  foreach(file cartcreates cartshift1 cartsuball cartzero)
    # not compiled files
    # cartmap1 dgraph_unwgt dims1 dims2 dims3 dims4 distgraph1 graphcr2 graphcr graphmap1 neighb_coll topodup topotest
    add_executable(${file} EXCLUDE_FROM_ALL ${file}.c)
    add_dependencies(tests ${file})
    target_link_libraries(${file} simgrid mtest_c)
  endforeach()
endif()

foreach(file cartcreates cartshift1 cartsuball cartzero cartmap1 dgraph_unwgt dims1 dims2 dims3 dims4 dims5 distgraph1
        graphcr2 graphcr graphmap1 neighb_coll topodup topotest)
  set(examples_src  ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${file}.c)
endforeach()

if (enable_smpi_MPICH3_testsuite AND HAVE_RAW_CONTEXTS)
  ADD_TEST(test-smpi-mpich3-topo-raw       ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/topo ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests "-wrapper=${TESH_WRAPPER}" -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/topo -tests=testlist -execarg=--cfg=contexts/factory:raw)
  SET_TESTS_PROPERTIES(test-smpi-mpich3-topo-raw PROPERTIES PASS_REGULAR_EXPRESSION "tests passed!")
endif()

set(examples_src  ${examples_src}                                        PARENT_SCOPE)
set(txt_files     ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/testlist  PARENT_SCOPE)
