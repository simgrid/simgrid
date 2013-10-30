cmake_minimum_required(VERSION 2.6)

if(enable_smpi AND enable_smpi_MPICH3_testsuite)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
  include_directories("${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

  add_executable(blockindexed-misc blockindexed-misc.c ../util/mtest.c)
  add_executable(blockindexed-zero-count blockindexed-zero-count.c ../util/mtest.c)
  add_executable(contents contents.c ../util/mtest.c)
  add_executable(contigstruct contigstruct.c ../util/mtest.c)
  add_executable(contig-zero-count contig-zero-count.c ../util/mtest.c)
  add_executable(cxx-types cxx-types.c ../util/mtest.c)
  add_executable(darray-cyclic darray-cyclic.c ../util/mtest.c)
  add_executable(darray-pack darray-pack.c ../util/mtest.c)
  add_executable(gaddress gaddress.c ../util/mtest.c)
  add_executable(get-elements get-elements.c ../util/mtest.c)
  add_executable(get-elements-pairtype get-elements-pairtype.c ../util/mtest.c)
  add_executable(getpartelm getpartelm.c ../util/mtest.c)
  add_executable(hindexed_block hindexed_block.c ../util/mtest.c)
  add_executable(hindexed_block_contents hindexed_block_contents.c ../util/mtest.c)
  add_executable(hindexed-zeros hindexed-zeros.c ../util/mtest.c)
  add_executable(indexed-misc indexed-misc.c ../util/mtest.c)
  add_executable(large-count large-count.c ../util/mtest.c)
  add_executable(lbub lbub.c ../util/mtest.c)
  add_executable(localpack localpack.c ../util/mtest.c)
  add_executable(longdouble longdouble.c ../util/mtest.c)
  add_executable(lots-of-types lots-of-types.c ../util/mtest.c)
  add_executable(pairtype-pack pairtype-pack.c ../util/mtest.c)
  add_executable(pairtype-size-extent pairtype-size-extent.c ../util/mtest.c)
  add_executable(simple-commit simple-commit.c ../util/mtest.c)
  add_executable(simple-pack simple-pack.c ../util/mtest.c)
  add_executable(simple-pack-external simple-pack-external.c ../util/mtest.c)
  add_executable(simple-resized simple-resized.c ../util/mtest.c)
  add_executable(simple-size-extent simple-size-extent.c ../util/mtest.c)
  add_executable(sizedtypes sizedtypes.c ../util/mtest.c)
  add_executable(slice-pack slice-pack.c ../util/mtest.c)
  add_executable(slice-pack-external slice-pack-external.c ../util/mtest.c)
  add_executable(struct-derived-zeros struct-derived-zeros.c ../util/mtest.c)
  add_executable(struct-empty-el struct-empty-el.c ../util/mtest.c)
  add_executable(struct-ezhov struct-ezhov.c ../util/mtest.c)
  add_executable(struct-no-real-types struct-no-real-types.c ../util/mtest.c)
  add_executable(struct-pack struct-pack.c ../util/mtest.c)
  add_executable(struct-verydeep struct-verydeep.c ../util/mtest.c)
  add_executable(struct-zero-count struct-zero-count.c ../util/mtest.c)
  add_executable(subarray subarray.c ../util/mtest.c)
  add_executable(subarray-pack subarray-pack.c ../util/mtest.c)
  add_executable(tfree tfree.c ../util/mtest.c)
  add_executable(tmatchsize tmatchsize.c ../util/mtest.c)
  add_executable(transpose-pack transpose-pack.c ../util/mtest.c)
  add_executable(tresized2 tresized2.c ../util/mtest.c)
  add_executable(tresized tresized.c ../util/mtest.c)
  add_executable(triangular-pack triangular-pack.c ../util/mtest.c)
  add_executable(typecommit typecommit.c ../util/mtest.c)
  add_executable(typefree typefree.c ../util/mtest.c)
  add_executable(typelb typelb.c ../util/mtest.c)
  add_executable(typename typename.c ../util/mtest.c)
  add_executable(unpack unpack.c ../util/mtest.c)
  add_executable(unusual-noncontigs unusual-noncontigs.c ../util/mtest.c)
  add_executable(zero-blklen-vector zero-blklen-vector.c ../util/mtest.c)
  add_executable(zeroblks zeroblks.c ../util/mtest.c)
  add_executable(zeroparms zeroparms.c ../util/mtest.c)

  target_link_libraries(blockindexed-misc  simgrid)
  target_link_libraries(blockindexed-zero-count  simgrid)
  target_link_libraries(contents  simgrid)
  target_link_libraries(contigstruct  simgrid)
  target_link_libraries(contig-zero-count  simgrid)
  target_link_libraries(cxx-types  simgrid)
  target_link_libraries(darray-cyclic  simgrid)
  target_link_libraries(darray-pack  simgrid)
  target_link_libraries(gaddress  simgrid)
  target_link_libraries(get-elements  simgrid)
  target_link_libraries(get-elements-pairtype  simgrid)
  target_link_libraries(getpartelm  simgrid)
  target_link_libraries(hindexed_block  simgrid)
  target_link_libraries(hindexed_block_contents  simgrid)
  target_link_libraries(hindexed-zeros  simgrid)
  target_link_libraries(indexed-misc  simgrid)
  target_link_libraries(large-count  simgrid)
  target_link_libraries(lbub  simgrid)
  target_link_libraries(localpack  simgrid)
  target_link_libraries(longdouble  simgrid)
  target_link_libraries(lots-of-types  simgrid)
  target_link_libraries(pairtype-pack  simgrid)
  target_link_libraries(pairtype-size-extent  simgrid)
  target_link_libraries(simple-commit  simgrid)
  target_link_libraries(simple-pack  simgrid)
  target_link_libraries(simple-pack-external  simgrid)
  target_link_libraries(simple-resized  simgrid)
  target_link_libraries(simple-size-extent  simgrid)
  target_link_libraries(sizedtypes  simgrid)
  target_link_libraries(slice-pack  simgrid)
  target_link_libraries(slice-pack-external  simgrid)
  target_link_libraries(struct-derived-zeros  simgrid)
  target_link_libraries(struct-empty-el  simgrid)
  target_link_libraries(struct-ezhov  simgrid)
  target_link_libraries(struct-no-real-types  simgrid)
  target_link_libraries(struct-pack  simgrid)
  target_link_libraries(struct-verydeep  simgrid)
  target_link_libraries(struct-zero-count  simgrid)
  target_link_libraries(subarray  simgrid)
  target_link_libraries(subarray-pack  simgrid)
  target_link_libraries(tfree  simgrid)
  target_link_libraries(tmatchsize  simgrid)
  target_link_libraries(transpose-pack  simgrid)
  target_link_libraries(tresized2  simgrid)
  target_link_libraries(tresized  simgrid)
  target_link_libraries(triangular-pack  simgrid)
  target_link_libraries(typecommit  simgrid)
  target_link_libraries(typefree  simgrid)
  target_link_libraries(typelb  simgrid)
  target_link_libraries(typename  simgrid)
  target_link_libraries(unpack  simgrid)
  target_link_libraries(unusual-noncontigs  simgrid)
  target_link_libraries(zero-blklen-vector  simgrid)
  target_link_libraries(zeroblks  simgrid)
  target_link_libraries(zeroparms  simgrid)

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
 ${CMAKE_CURRENT_SOURCE_DIR}/blockindexed-misc.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/blockindexed-zero-count.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/contents.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/contigstruct.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/contig-zero-count.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/cxx-types.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/darray-cyclic.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/darray-pack.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/gaddress.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/get-elements.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/get-elements-pairtype.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/getpartelm.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/hindexed_block.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/hindexed_block_contents.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/hindexed-zeros.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/indexed-misc.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/large-count.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/lbub.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/localpack.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/longdouble.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/lots-of-types.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/pairtype-pack.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/pairtype-size-extent.c 
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
 ${CMAKE_CURRENT_SOURCE_DIR}/zero-blklen-vector.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/zeroblks.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/zeroparms.c 
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE
  )
