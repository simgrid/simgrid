if(enable_smpi AND enable_smpi_MPICH3_testsuite)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

#  add_executable(blockindexed-misc blockindexed-misc.c)
  add_executable(blockindexed-zero-count blockindexed-zero-count.c)
#  add_executable(contents contents.c)
  add_executable(contigstruct contigstruct.c)
  add_executable(contig-zero-count contig-zero-count.c)
  add_executable(cxx-types cxx-types.c)
#  add_executable(darray-cyclic darray-cyclic.c)
#  add_executable(darray-pack darray-pack.c)
  add_executable(dataalign dataalign.c)
  add_executable(gaddress gaddress.c)
#  add_executable(get-elements get-elements.c)
#  add_executable(get-elements-pairtype get-elements-pairtype.c)
#  add_executable(getpartelm getpartelm.c)
#  add_executable(get-struct get-struct.c)
  add_executable(hindexed_block hindexed_block.c)
  add_executable(hindexed_block_contents hindexed_block_contents.c)
#  add_executable(hindexed-zeros hindexed-zeros.c)
#  add_executable(hvecblklen hvecblklen.c)
#  add_executable(indexed-misc indexed-misc.c)
#  add_executable(large-count large-count.c)
#  add_executable(large_type large_type.c)
#  add_executable(large_type_sendrec large_type_sendrec.c)
#  add_executable(lbub lbub.c)
  add_executable(localpack localpack.c)
  add_executable(longdouble longdouble.c)
#  add_executable(lots-of-types lots-of-types.c)
#  add_executable(pairtype-pack pairtype-pack.c)
#  add_executable(pairtype-size-extent pairtype-size-extent.c)
#  add_executable(segtest segtest.c)
  add_executable(sendrecvt2 ${CMAKE_CURRENT_SOURCE_DIR}/../util/dtypes.c sendrecvt2.c)
  add_executable(sendrecvt4 ${CMAKE_CURRENT_SOURCE_DIR}/../util/dtypes.c sendrecvt4.c)
  add_executable(simple-commit simple-commit.c)
  add_executable(simple-pack simple-pack.c)
#  add_executable(simple-pack-external simple-pack-external.c)
  add_executable(simple-resized simple-resized.c)
  add_executable(simple-size-extent simple-size-extent.c)
#  add_executable(sizedtypes sizedtypes.c)
#  add_executable(slice-pack slice-pack.c)
#  add_executable(slice-pack-external slice-pack-external.c)
  add_executable(struct-derived-zeros struct-derived-zeros.c)
#  add_executable(struct-empty-el struct-empty-el.c)
  add_executable(struct-ezhov struct-ezhov.c)
#  add_executable(struct-no-real-types struct-no-real-types.c)
  add_executable(struct-pack struct-pack.c)
#  add_executable(structpack2 structpack2.c)
  add_executable(struct-verydeep struct-verydeep.c)
  add_executable(struct-zero-count struct-zero-count.c)
#  add_executable(subarray subarray.c)
#  add_executable(subarray-pack subarray-pack.c)
  add_executable(tfree tfree.c)
#  add_executable(tmatchsize tmatchsize.c)
  add_executable(transpose-pack transpose-pack.c)
  add_executable(tresized2 tresized2.c)
  add_executable(tresized tresized.c)
#  add_executable(triangular-pack triangular-pack.c)
  add_executable(typecommit typecommit.c)
  add_executable(typefree typefree.c)
  add_executable(typelb typelb.c)
  add_executable(typename typename.c)
  add_executable(unpack unpack.c)
#  add_executable(unusual-noncontigs unusual-noncontigs.c)
#  add_executable(vecblklen vecblklen.c)
#  add_executable(zero-blklen-vector zero-blklen-vector.c)
  add_executable(zeroblks zeroblks.c)
  add_executable(zeroparms zeroparms.c)

#  target_link_libraries(blockindexed-misc simgrid mtest_c)
  target_link_libraries(blockindexed-zero-count simgrid mtest_c)
#  target_link_libraries(contents simgrid mtest_c)
  target_link_libraries(contigstruct simgrid mtest_c)
  target_link_libraries(contig-zero-count simgrid mtest_c)
  target_link_libraries(cxx-types simgrid mtest_c)
#  target_link_libraries(darray-cyclic simgrid mtest_c)
#  target_link_libraries(darray-pack simgrid mtest_c)
  target_link_libraries(dataalign simgrid mtest_c)
  target_link_libraries(gaddress simgrid mtest_c)
#  target_link_libraries(get-elements simgrid mtest_c)
#  target_link_libraries(get-elements-pairtype simgrid mtest_c)
#  target_link_libraries(getpartelm simgrid mtest_c)
#  target_link_libraries(get-struct simgrid mtest_c)
  target_link_libraries(hindexed_block simgrid mtest_c)
  target_link_libraries(hindexed_block_contents simgrid mtest_c)
#  target_link_libraries(hindexed-zeros simgrid mtest_c).
#  target_link_libraries(hvecblklen simgrid mtest_c)
#  target_link_libraries(indexed-misc simgrid mtest_c)
#  target_link_libraries(large-count simgrid mtest_c)
#  target_link_libraries(large_type simgrid mtest_c)
#  target_link_libraries(large_type_sendrec simgrid mtest_c)
#  target_link_libraries(lbub simgrid mtest_c)
  target_link_libraries(localpack simgrid mtest_c)
  target_link_libraries(longdouble simgrid mtest_c)
#  target_link_libraries(lots-of-types simgrid mtest_c)
#  target_link_libraries(pairtype-pack simgrid mtest_c)
#  target_link_libraries(pairtype-size-extent simgrid mtest_c)
#  target_link_libraries(segtest simgrid mtest_c)
  target_link_libraries(sendrecvt2 simgrid mtest_c)
  target_link_libraries(sendrecvt4 simgrid mtest_c)
  target_link_libraries(simple-commit simgrid mtest_c)
  target_link_libraries(simple-pack simgrid mtest_c)
#  target_link_libraries(simple-pack-external simgrid mtest_c)
  target_link_libraries(simple-resized simgrid mtest_c)
  target_link_libraries(simple-size-extent simgrid mtest_c)
#  target_link_libraries(sizedtypes simgrid mtest_c)
#  target_link_libraries(slice-pack simgrid mtest_c)
#  target_link_libraries(slice-pack-external simgrid mtest_c)
  target_link_libraries(struct-derived-zeros simgrid mtest_c)
#  target_link_libraries(struct-empty-el simgrid mtest_c)
  target_link_libraries(struct-ezhov simgrid mtest_c)
#  target_link_libraries(struct-no-real-types simgrid mtest_c)
  target_link_libraries(struct-pack simgrid mtest_c)
#  target_link_libraries(structpack2 simgrid mtest_c)
  target_link_libraries(struct-verydeep simgrid mtest_c)
  target_link_libraries(struct-zero-count simgrid mtest_c)
#  target_link_libraries(subarray simgrid mtest_c)
#  target_link_libraries(subarray-pack simgrid mtest_c)
  target_link_libraries(tfree simgrid mtest_c)
#  target_link_libraries(tmatchsize simgrid mtest_c)
  target_link_libraries(transpose-pack simgrid mtest_c)
  target_link_libraries(tresized2 simgrid mtest_c)
  target_link_libraries(tresized simgrid mtest_c)
#  target_link_libraries(triangular-pack simgrid mtest_c)
  target_link_libraries(typecommit simgrid mtest_c)
  target_link_libraries(typefree simgrid mtest_c)
  target_link_libraries(typelb simgrid mtest_c)
  target_link_libraries(typename simgrid mtest_c)
  target_link_libraries(unpack simgrid mtest_c)
#  target_link_libraries(unusual-noncontigs simgrid mtest_c)
#  target_link_libraries(vecblklen simgrid mtest_c)
#  target_link_libraries(zero-blklen-vector simgrid mtest_c)
  target_link_libraries(zeroblks simgrid mtest_c)
  target_link_libraries(zeroparms simgrid mtest_c)

endif()

set(examples_src
  ${examples_src}
 ${CMAKE_CURRENT_SOURCE_DIR}/blockindexed-misc.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/blockindexed-zero-count.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/contents.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/contigstruct.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/contig-zero-count.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/cxx-types.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/darray-cyclic.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/darray-pack.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/dataalign.c
 ${CMAKE_CURRENT_SOURCE_DIR}/gaddress.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/get-elements.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/get-elements-pairtype.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/getpartelm.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/get-struct.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/hindexed_block.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/hindexed_block_contents.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/hindexed-zeros.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/hvecblklen.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/indexed-misc.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/large-count.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/large_type.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/large_type_sendrec.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/lbub.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/localpack.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/longdouble.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/lots-of-types.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/pairtype-pack.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/pairtype-size-extent.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/segtest.c
 ${CMAKE_CURRENT_SOURCE_DIR}/sendrecvt2.c
 ${CMAKE_CURRENT_SOURCE_DIR}/sendrecvt4.c
 ${CMAKE_CURRENT_SOURCE_DIR}/simple-commit.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/simple-pack.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/simple-pack-external.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/simple-resized.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/simple-size-extent.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/sizedtypes.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/slice-pack.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/slice-pack-external.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/struct-derived-zeros.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/struct-empty-el.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/struct-ezhov.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/struct-no-real-types.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/struct-pack.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/structpack2.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/struct-verydeep.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/struct-zero-count.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/subarray.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/subarray-pack.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/tfree.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/tmatchsize.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/transpose-pack.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/tresized2.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/tresized.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/triangular-pack.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/typecommit.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/typefree.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/typelb.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/typename.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/unpack.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/unusual-noncontigs.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/vecblklen.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/zero-blklen-vector.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/zeroblks.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/zeroparms.c 
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE)
