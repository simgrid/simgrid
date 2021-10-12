if(enable_smpi AND enable_smpi_MPICH3_testsuite)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

  foreach(file commcreatep non_zero_root sendrecvl timer transp-datatype twovec dtpack indexperf  nestvec2 nestvec)
    #not compiled files manyrma
    add_executable(${file} EXCLUDE_FROM_ALL ${file}.c)
    add_dependencies(tests ${file})
    target_link_libraries(${file} simgrid mtest_c)
  endforeach()
endif()

if (enable_smpi_MPICH3_testsuite AND HAVE_RAW_CONTEXTS)
  ADD_TEST(test-smpi-mpich3-perf-raw       ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/perf ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests "-wrapper=${TESH_WRAPPER}" -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -tests=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/perf/testlist -execarg=-platform\ ${CMAKE_HOME_DIRECTORY}/examples/platforms/cluster_backbone.xml -execarg=--log=root.thr:critical -execarg=--cfg=smpi/async-small-thresh:65536 -execarg=--cfg=contexts/factory:raw -execarg=--cfg=smpi/simulate-computation:0)
endif()

foreach(file allredtrace commcreatep non_zero_root sendrecvl timer transp-datatype twovec dtpack indexperf manyrma
        nestvec2 nestvec)
  set(examples_src  ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${file}.c)
endforeach()

set(examples_src  ${examples_src}                                        PARENT_SCOPE)
set(txt_files     ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/README
                                   ${CMAKE_CURRENT_SOURCE_DIR}/testlist  PARENT_SCOPE)
