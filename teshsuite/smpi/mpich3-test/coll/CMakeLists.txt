if(enable_smpi AND enable_smpi_MPICH3_testsuite)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

  foreach(test allgather2 allgather3 allgather_struct allgatherv2 allgatherv3
          allred2 allred3 allred4 allred5 allred6 allredmany alltoall1
          alltoallv0 alltoallv alltoallw1 alltoallw2 alltoallw_zeros
          bcasttest bcastzerotype coll2 coll3 coll4 coll5 coll6 coll7 coll8
          coll9 coll10 coll11 coll12 coll13 exscan exscan2 gather gather2
          gather_big ibarrier longuser nonblocking nonblocking2 nonblocking3 iallred
        # icallgather icallgatherv icallreduce
        # icalltoall icalltoallv icalltoallw icbarrier icbcast
        # icgather icgatherv icreduce icscatter icscatterv
         opband opbor opbxor opland oplor oplxor opmax opmaxloc
         opmin opminloc opprod opsum
         op_commutative red3 red4 redscat2 redscat3 redscatbkinter redscatblk3
         redscat red_scat_block red_scat_block2 allgatherv4 allred
        # redscatinter
          reduce_local scantst scatter2 scatter3 scattern scatterv
         uoplong
         )
    add_executable(${test} EXCLUDE_FROM_ALL ${test}.c)
    add_dependencies(tests ${test})
    target_link_libraries(${test} simgrid mtest_c)
  endforeach()

  set_target_properties(allred PROPERTIES COMPILE_FLAGS "-O0" LINK_FLAGS "-O0")

  foreach(test bcast_full bcast_min_datatypes bcast_comm_world)
    add_executable(${test} EXCLUDE_FROM_ALL bcast.c)
    add_dependencies(tests ${test})
    target_link_libraries(${test} simgrid mtest_c)
  endforeach()
  set_target_properties(bcast_min_datatypes PROPERTIES COMPILE_FLAGS "-DBCAST_MIN_DATATYPES_ONLY" LINK_FLAGS "-DBCAST_MIN_DATATYPES_ONLY")
  set_target_properties(bcast_comm_world PROPERTIES COMPILE_FLAGS "-DBCAST_COMM_WORLD_ONLY" LINK_FLAGS "-DBCAST_COMM_WORLD_ONLY")

  add_executable(reduce_mpich EXCLUDE_FROM_ALL reduce.c)
  add_dependencies(tests reduce_mpich)
  target_link_libraries(reduce_mpich  simgrid mtest_c)

  # These tests take 5 to 15 seconds to run, so we don't want to run them several times.
  # But at the same time, we'd like to check if they work for all factories and all privatization algorithm
  # Thus the current matrix

  MACRO(ADD_MPICH3_COLL SELECTOR FACTORY PRIVATIZATION)
    set(NAME "test-smpi-mpich3-coll-${SELECTOR}")
    set(ARGS "-execarg=--cfg=smpi/coll-selector:${SELECTOR}" ${ARGN})
    if(NOT "${PRIVATIZATION}" STREQUAL "" AND HAVE_PRIVATIZATION)
      set(NAME "${NAME}-${PRIVATIZATION}")
      set(ARGS ${ARGS} "-execarg=--cfg=smpi/privatization:${PRIVATIZATION}")
    endif()
    string(TOUPPER "HAVE_${FACTORY}_CONTEXTS" HAVE_FACTORY)
    if(NOT "${FACTORY}" STREQUAL "" AND ${HAVE_FACTORY})
      set(NAME "${NAME}-${FACTORY}")
      set(ARGS ${ARGS} "-execarg=--cfg=contexts/factory:${FACTORY}")
    endif()
    ADD_TEST(${NAME} ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/coll ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests "-wrapper=${VALGRIND_WRAPPER}" -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/coll -tests=testlist ${ARGS})
    SET_TESTS_PROPERTIES(${NAME} PROPERTIES PASS_REGULAR_EXPRESSION "tests passed!")
  ENDMACRO()

  # Test default selector; default factory; default privatization
  ADD_MPICH3_COLL(default "" "")

  # Test OMPI selector: thread factory, dlopen privatization
  ADD_MPICH3_COLL(ompi "thread" "dlopen" -execarg=--cfg=smpi/bcast:binomial_tree)

  # Test MPICH selector: boost factory, dlopen privatization
  ADD_MPICH3_COLL(mpich "boost" "dlopen")

  # Test MVAPICH2 selector: ucontext factory, mmap privatization
  ADD_MPICH3_COLL(mvapich2 "ucontext" "mmap")

  # Test IMPI selector: raw factory, mmap privatization
  ADD_MPICH3_COLL(impi "raw" "mmap")

endif()

set(examples_src  ${examples_src}
 ${CMAKE_CURRENT_SOURCE_DIR}/allgather2.c
 ${CMAKE_CURRENT_SOURCE_DIR}/allgather_struct.c
 ${CMAKE_CURRENT_SOURCE_DIR}/allgather3.c
 ${CMAKE_CURRENT_SOURCE_DIR}/allgatherv2.c
 ${CMAKE_CURRENT_SOURCE_DIR}/allgatherv3.c
 ${CMAKE_CURRENT_SOURCE_DIR}/allgatherv4.c
 ${CMAKE_CURRENT_SOURCE_DIR}/allred2.c
 ${CMAKE_CURRENT_SOURCE_DIR}/allred3.c
 ${CMAKE_CURRENT_SOURCE_DIR}/allred4.c
 ${CMAKE_CURRENT_SOURCE_DIR}/allred5.c
 ${CMAKE_CURRENT_SOURCE_DIR}/allred6.c
 ${CMAKE_CURRENT_SOURCE_DIR}/allred.c
 ${CMAKE_CURRENT_SOURCE_DIR}/allredmany.c
 ${CMAKE_CURRENT_SOURCE_DIR}/alltoall1.c
 ${CMAKE_CURRENT_SOURCE_DIR}/alltoallv0.c
 ${CMAKE_CURRENT_SOURCE_DIR}/alltoallv.c
 ${CMAKE_CURRENT_SOURCE_DIR}/alltoallw1.c
 ${CMAKE_CURRENT_SOURCE_DIR}/alltoallw2.c
 ${CMAKE_CURRENT_SOURCE_DIR}/alltoallw_zeros.c
 ${CMAKE_CURRENT_SOURCE_DIR}/bcast.c
 ${CMAKE_CURRENT_SOURCE_DIR}/bcasttest.c
 ${CMAKE_CURRENT_SOURCE_DIR}/bcastzerotype.c
 ${CMAKE_CURRENT_SOURCE_DIR}/coll10.c
 ${CMAKE_CURRENT_SOURCE_DIR}/coll11.c
 ${CMAKE_CURRENT_SOURCE_DIR}/coll12.c
 ${CMAKE_CURRENT_SOURCE_DIR}/coll13.c
 ${CMAKE_CURRENT_SOURCE_DIR}/coll2.c
 ${CMAKE_CURRENT_SOURCE_DIR}/coll3.c
 ${CMAKE_CURRENT_SOURCE_DIR}/coll4.c
 ${CMAKE_CURRENT_SOURCE_DIR}/coll5.c
 ${CMAKE_CURRENT_SOURCE_DIR}/coll6.c
 ${CMAKE_CURRENT_SOURCE_DIR}/coll7.c
 ${CMAKE_CURRENT_SOURCE_DIR}/coll8.c
 ${CMAKE_CURRENT_SOURCE_DIR}/coll9.c
 ${CMAKE_CURRENT_SOURCE_DIR}/exscan2.c
 ${CMAKE_CURRENT_SOURCE_DIR}/exscan.c
 ${CMAKE_CURRENT_SOURCE_DIR}/gather2.c
 ${CMAKE_CURRENT_SOURCE_DIR}/gather_big.c
 ${CMAKE_CURRENT_SOURCE_DIR}/gather.c
 ${CMAKE_CURRENT_SOURCE_DIR}/iallred.c
 ${CMAKE_CURRENT_SOURCE_DIR}/ibarrier.c
 ${CMAKE_CURRENT_SOURCE_DIR}/icallgather.c
 ${CMAKE_CURRENT_SOURCE_DIR}/icallgatherv.c
 ${CMAKE_CURRENT_SOURCE_DIR}/icallreduce.c
 ${CMAKE_CURRENT_SOURCE_DIR}/icalltoall.c
 ${CMAKE_CURRENT_SOURCE_DIR}/icalltoallv.c
 ${CMAKE_CURRENT_SOURCE_DIR}/icalltoallw.c
 ${CMAKE_CURRENT_SOURCE_DIR}/icbarrier.c
 ${CMAKE_CURRENT_SOURCE_DIR}/icbcast.c
 ${CMAKE_CURRENT_SOURCE_DIR}/icgather.c
 ${CMAKE_CURRENT_SOURCE_DIR}/icgatherv.c
 ${CMAKE_CURRENT_SOURCE_DIR}/icreduce.c
 ${CMAKE_CURRENT_SOURCE_DIR}/icscatter.c
 ${CMAKE_CURRENT_SOURCE_DIR}/icscatterv.c
 ${CMAKE_CURRENT_SOURCE_DIR}/longuser.c
 ${CMAKE_CURRENT_SOURCE_DIR}/nonblocking2.c
 ${CMAKE_CURRENT_SOURCE_DIR}/nonblocking3.c
 ${CMAKE_CURRENT_SOURCE_DIR}/nonblocking.c
 ${CMAKE_CURRENT_SOURCE_DIR}/opband.c
 ${CMAKE_CURRENT_SOURCE_DIR}/opbor.c
 ${CMAKE_CURRENT_SOURCE_DIR}/opbxor.c
 ${CMAKE_CURRENT_SOURCE_DIR}/op_commutative.c
 ${CMAKE_CURRENT_SOURCE_DIR}/opland.c
 ${CMAKE_CURRENT_SOURCE_DIR}/oplor.c
 ${CMAKE_CURRENT_SOURCE_DIR}/oplxor.c
 ${CMAKE_CURRENT_SOURCE_DIR}/opmax.c
 ${CMAKE_CURRENT_SOURCE_DIR}/opmaxloc.c
 ${CMAKE_CURRENT_SOURCE_DIR}/opmin.c
 ${CMAKE_CURRENT_SOURCE_DIR}/opminloc.c
 ${CMAKE_CURRENT_SOURCE_DIR}/opprod.c
 ${CMAKE_CURRENT_SOURCE_DIR}/opsum.c
 ${CMAKE_CURRENT_SOURCE_DIR}/red3.c
 ${CMAKE_CURRENT_SOURCE_DIR}/red4.c
 ${CMAKE_CURRENT_SOURCE_DIR}/redscat2.c
 ${CMAKE_CURRENT_SOURCE_DIR}/redscat3.c
 ${CMAKE_CURRENT_SOURCE_DIR}/redscatbkinter.c
 ${CMAKE_CURRENT_SOURCE_DIR}/redscatblk3.c
 ${CMAKE_CURRENT_SOURCE_DIR}/red_scat_block2.c
 ${CMAKE_CURRENT_SOURCE_DIR}/red_scat_block.c
 ${CMAKE_CURRENT_SOURCE_DIR}/redscat.c
 ${CMAKE_CURRENT_SOURCE_DIR}/redscatinter.c
 ${CMAKE_CURRENT_SOURCE_DIR}/reduce.c
 ${CMAKE_CURRENT_SOURCE_DIR}/reduce_local.c
 ${CMAKE_CURRENT_SOURCE_DIR}/scantst.c
 ${CMAKE_CURRENT_SOURCE_DIR}/scatter2.c
 ${CMAKE_CURRENT_SOURCE_DIR}/scatter3.c
 ${CMAKE_CURRENT_SOURCE_DIR}/scattern.c
 ${CMAKE_CURRENT_SOURCE_DIR}/scatterv.c
 ${CMAKE_CURRENT_SOURCE_DIR}/uoplong.c
  PARENT_SCOPE)
set(txt_files  ${txt_files}  ${CMAKE_CURRENT_SOURCE_DIR}/testlist  PARENT_SCOPE)
