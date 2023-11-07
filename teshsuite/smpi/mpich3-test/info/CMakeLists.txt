if(enable_smpi AND enable_testsuite_smpi_MPICH3)
  set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

  foreach(file infodel infodup infomany2 infomany infoorder infotest infoget infovallen infoenv)
    add_executable(${file} EXCLUDE_FROM_ALL ${file}.c)
    add_dependencies(tests ${file})
    target_link_libraries(${file} simgrid mtest_c)
  endforeach()
endif()

if (enable_testsuite_smpi_MPICH3 AND HAVE_RAW_CONTEXTS)
  ADD_TEST(test-smpi-mpich3-info-raw       ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/info ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests "-wrapper=${VALGRIND_WRAPPER}" -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/info -tests=testlist -execarg=--cfg=contexts/factory:raw)
  SET_TESTS_PROPERTIES(test-smpi-mpich3-info-raw PROPERTIES PASS_REGULAR_EXPRESSION "tests passed!")
endif()

foreach(file infodel infodup infomany2 infomany infoorder infotest infoget infovallen infoenv)
  set(examples_src  ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${file}.c)
endforeach()

set(examples_src  ${examples_src}                                        PARENT_SCOPE)
set(txt_files     ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/testlist  PARENT_SCOPE)
