if(enable_smpi AND enable_smpi_MPICH3_testsuite)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

  foreach(file blockindexed-zero-count contigstruct contig-zero-count cxx-types dataalign gaddress hindexed_block
          hindexed_block_contents hvecblklen large_vec localpack longdouble simple-commit simple-pack simple-resized simple-size-extent
          struct-derived-zeros struct-ezhov struct-pack struct-verydeep struct-zero-count tfree transpose-pack tresized2
          tresized typecommit typefree typelb typename unpack vecblklen zeroblks zeroparms
          subarray subarray-pack contents)
    # not compiled files
    # blockindexed-misc darray-cyclic darray-pack get-elements get-elements-pairtype getpartelm get-struct
    # hindexed-zeros indexed-misc large-count large_type large_type_sendrec lbub lots-of-type pairtype-pack
    # pairtype-size-extent segtest simple-pack-external simple-pack-external2 sizedtypes slice-pack slice-pack-external struct-empty-el
    # struct-no-real-type structpack2 subarray subarray-pack tmatchsize triangular-pack unusual-noncontigs
    # zero-blklen-vector
    add_executable(${file} EXCLUDE_FROM_ALL ${file}.c)
    add_dependencies(tests ${file})
    target_link_libraries(${file} simgrid mtest_c)
  endforeach()

  add_executable(sendrecvt2 EXCLUDE_FROM_ALL ${CMAKE_CURRENT_SOURCE_DIR}/../util/dtypes.c sendrecvt2.c)
  add_executable(sendrecvt4 EXCLUDE_FROM_ALL ${CMAKE_CURRENT_SOURCE_DIR}/../util/dtypes.c sendrecvt4.c)
  add_dependencies(tests sendrecvt2 sendrecvt4)
  target_link_libraries(sendrecvt2 simgrid mtest_c)
  target_link_libraries(sendrecvt4 simgrid mtest_c)
  set_target_properties(hindexed_block PROPERTIES COMPILE_FLAGS "-DTEST_HINDEXED_BLOCK" LINK_FLAGS "-DTEST_HINDEXED_BLOCK")


endif()

if (enable_smpi_MPICH3_testsuite AND HAVE_RAW_CONTEXTS)
  ADD_TEST(test-smpi-mpich3-datatype-raw   ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/datatype ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests "-wrapper=${VALGRIND_WRAPPER}" -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/datatype -tests=testlist -execarg=--cfg=contexts/factory:raw)
  SET_TESTS_PROPERTIES(test-smpi-mpich3-datatype-raw PROPERTIES PASS_REGULAR_EXPRESSION "tests passed!")
endif()

foreach(file blockindexed-misc blockindexed-zero-count contents contigstruct contig-zero-count cxx-types darray-cyclic
        darray-pack dataalign gaddress get-elements get-elements-pairtype getpartelm hindexed_block
        hindexed_block_contents hindexed-zeros hvecblklen indexed-misc large-count large_type large_type_sendrec lbub
        localpack longdouble lots-of-types pairtype-pack pairtype-size-extent segtest sendrecvt2 sendrecvt4
        simple-commit simple-pack simple-pack-external simple-pack-external2 simple-resized simple-size-extent sizedtypes slice-pack
        slice-pack-external large_vec struct-derived-zeros struct-empty-el struct-ezhov struct-no-real-types struct-pack
        structpack2 struct-verydeep struct-zero-count subarray subarray-pack tfree tmatchsize transpose-pack tresized2
        tresized triangular-pack typecommit typefree typelb typename unpack unusual-noncontigs vecblklen
        zero-blklen-vector zeroblks zeroparms)
  set(examples_src  ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${file}.c)
endforeach()

set(examples_src  ${examples_src}                                        PARENT_SCOPE)
set(txt_files     ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/testlist  PARENT_SCOPE)
