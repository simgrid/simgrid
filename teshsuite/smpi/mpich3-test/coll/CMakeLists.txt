if(enable_smpi AND enable_smpi_MPICH3_testsuite)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

  add_executable(allgather2 allgather2.c)
  add_executable(allgather3 allgather3.c)
  add_executable(allgatherv2 allgatherv2.c)
  add_executable(allgatherv3 allgatherv3.c)
  if(HAVE_PRIVATIZATION)
    add_executable(allgatherv4 allgatherv4.c)
  else()
    add_executable(allgatherv4 allgatherv4_manual.c)
  endif()
  add_executable(allred2 allred2.c)
  add_executable(allred3 allred3.c)
  add_executable(allred4 allred4.c)
  add_executable(allred5 allred5.c)
  add_executable(allred6 allred6.c)
  add_executable(allred allred.c)
  add_executable(allredmany allredmany.c)
  add_executable(alltoall1 alltoall1.c)
  add_executable(alltoallv0 alltoallv0.c)
  add_executable(alltoallv alltoallv.c)
#  add_executable(alltoallw1 alltoallw1.c)
#  add_executable(alltoallw2 alltoallw2.c)
#  add_executable(alltoallw_zeros alltoallw_zeros.c)
  add_executable(bcast2 bcast2.c)
  add_executable(bcast3 bcast3.c)
  add_executable(bcasttest bcasttest.c)
  add_executable(bcastzerotype bcastzerotype.c)
  add_executable(coll10 coll10.c)
  add_executable(coll11 coll11.c)
  add_executable(coll12 coll12.c)
  add_executable(coll13 coll13.c)
  add_executable(coll2 coll2.c)
  add_executable(coll3 coll3.c)
  add_executable(coll4 coll4.c)
  add_executable(coll5 coll5.c)
  add_executable(coll6 coll6.c)
  add_executable(coll7 coll7.c)
  add_executable(coll8 coll8.c)
  add_executable(coll9 coll9.c)
  add_executable(exscan2 exscan2.c)
  add_executable(exscan exscan.c)
  add_executable(gather2 gather2.c)
#  add_executable(gather2_save gather2_save.c)
  add_executable(gather gather.c)
#  add_executable(iallred iallred.c)
#  add_executable(ibarrier ibarrier.c)
#  add_executable(icallgather icallgather.c)
#  add_executable(icallgatherv icallgatherv.c)
#  add_executable(icallreduce icallreduce.c)
#  add_executable(icalltoall icalltoall.c)
#  add_executable(icalltoallv icalltoallv.c)
#  add_executable(icalltoallw icalltoallw.c)
#  add_executable(icbarrier icbarrier.c)
#  add_executable(icbcast icbcast.c)
#  add_executable(icgather icgather.c)
#  add_executable(icgatherv icgatherv.c)
#  add_executable(icreduce icreduce.c)
#  add_executable(icscatter icscatter.c)
#  add_executable(icscatterv icscatterv.c)
  add_executable(longuser longuser.c)
#  add_executable(nonblocking2 nonblocking2.c)
#  add_executable(nonblocking3 nonblocking3.c)
#  add_executable(nonblocking4 nonblocking4.c)
#  add_executable(nonblocking nonblocking.c)
#  add_executable(opband opband.c)
#  add_executable(opbor opbor.c)
#  add_executable(opbxor opbxor.c)
  add_executable(op_commutative op_commutative.c)
#  add_executable(opland opland.c)
#  add_executable(oplor oplor.c)
#  add_executable(oplxor oplxor.c)
#  add_executable(opmax opmax.c)
#  add_executable(opmaxloc opmaxloc.c)
#  add_executable(opmin opmin.c)
#  add_executable(opminloc opminloc.c)
#  add_executable(opprod opprod.c)
#  add_executable(opsum opsum.c)
  add_executable(red3 red3.c)
  add_executable(red4 red4.c)
  add_executable(redscat2 redscat2.c)
  add_executable(redscat3 redscat3.c)
  add_executable(redscatbkinter redscatbkinter.c)
  add_executable(redscatblk3 redscatblk3.c)
  add_executable(red_scat_block2 red_scat_block2.c)
  add_executable(red_scat_block red_scat_block.c)
  add_executable(redscat redscat.c)
#  add_executable(redscatinter redscatinter.c)
  add_executable(reduce_mpich reduce.c)
  add_executable(reduce_local reduce_local.c)
  add_executable(scantst scantst.c)
  add_executable(scatter2 scatter2.c)
  add_executable(scatter3 scatter3.c)
  add_executable(scattern scattern.c)
  add_executable(scatterv scatterv.c)
#  add_executable(uoplong uoplong.c)

  target_link_libraries(allgather2  simgrid mtest_c)
  target_link_libraries(allgather3  simgrid mtest_c)
  target_link_libraries(allgatherv2  simgrid mtest_c)
  target_link_libraries(allgatherv3  simgrid mtest_c)
  target_link_libraries(allgatherv4  simgrid mtest_c)
  target_link_libraries(allred2  simgrid mtest_c)
  target_link_libraries(allred3  simgrid mtest_c)
  target_link_libraries(allred4  simgrid mtest_c)
  target_link_libraries(allred5  simgrid mtest_c)
  target_link_libraries(allred6  simgrid mtest_c)
  target_link_libraries(allred  simgrid mtest_c)
  target_link_libraries(allredmany  simgrid mtest_c)
  target_link_libraries(alltoall1  simgrid mtest_c)
  target_link_libraries(alltoallv0  simgrid mtest_c)
  target_link_libraries(alltoallv  simgrid mtest_c)
#  target_link_libraries(alltoallw1  simgrid mtest_c)
#  target_link_libraries(alltoallw2  simgrid mtest_c)
#  target_link_libraries(alltoallw_zeros  simgrid mtest_c)
  target_link_libraries(bcast2  simgrid mtest_c)
  target_link_libraries(bcast3  simgrid mtest_c)
  target_link_libraries(bcasttest  simgrid mtest_c)
  target_link_libraries(bcastzerotype  simgrid mtest_c)
  target_link_libraries(coll10  simgrid mtest_c)
  target_link_libraries(coll11  simgrid mtest_c)
  target_link_libraries(coll12  simgrid mtest_c)
  target_link_libraries(coll13  simgrid mtest_c)
  target_link_libraries(coll2  simgrid mtest_c)
  target_link_libraries(coll3  simgrid mtest_c)
  target_link_libraries(coll4  simgrid mtest_c)
  target_link_libraries(coll5  simgrid mtest_c)
  target_link_libraries(coll6  simgrid mtest_c)
  target_link_libraries(coll7  simgrid mtest_c)
  target_link_libraries(coll8  simgrid mtest_c)
  target_link_libraries(coll9  simgrid mtest_c)
  target_link_libraries(exscan2  simgrid mtest_c)
  target_link_libraries(exscan  simgrid mtest_c)
  target_link_libraries(gather2  simgrid mtest_c)
#  target_link_libraries(gather2_save  simgrid mtest_c)
  target_link_libraries(gather  simgrid mtest_c)
#  target_link_libraries(iallred  simgrid mtest_c)
#  target_link_libraries(ibarrier  simgrid mtest_c)
#  target_link_libraries(icallgather  simgrid mtest_c)
#  target_link_libraries(icallgatherv  simgrid mtest_c)
#  target_link_libraries(icallreduce  simgrid mtest_c)
#  target_link_libraries(icalltoall  simgrid mtest_c)
#  target_link_libraries(icalltoallv  simgrid mtest_c)
#  target_link_libraries(icalltoallw  simgrid mtest_c)
#  target_link_libraries(icbarrier  simgrid mtest_c)
#  target_link_libraries(icbcast  simgrid mtest_c)
#  target_link_libraries(icgather  simgrid mtest_c)
#  target_link_libraries(icgatherv  simgrid mtest_c)
#  target_link_libraries(icreduce  simgrid mtest_c)
#  target_link_libraries(icscatter  simgrid mtest_c)
#  target_link_libraries(icscatterv  simgrid mtest_c)
  target_link_libraries(longuser  simgrid mtest_c)
#  target_link_libraries(nonblocking2  simgrid mtest_c)
#  target_link_libraries(nonblocking3  simgrid mtest_c)
#  target_link_libraries(nonblocking4  simgrid mtest_c)
#  target_link_libraries(nonblocking  simgrid mtest_c)
#  target_link_libraries(opband  simgrid mtest_c)
#  target_link_libraries(opbor  simgrid mtest_c)
#  target_link_libraries(opbxor  simgrid mtest_c)
  target_link_libraries(op_commutative  simgrid mtest_c)
#  target_link_libraries(opland  simgrid mtest_c)
#  target_link_libraries(oplor  simgrid mtest_c)
#  target_link_libraries(oplxor  simgrid mtest_c)
#  target_link_libraries(opmax  simgrid mtest_c)
#  target_link_libraries(opmaxloc  simgrid mtest_c)
#  target_link_libraries(opmin  simgrid mtest_c)
#  target_link_libraries(opminloc  simgrid mtest_c)
#  target_link_libraries(opprod  simgrid mtest_c)
#  target_link_libraries(opsum  simgrid mtest_c)
  target_link_libraries(red3  simgrid mtest_c)
  target_link_libraries(red4  simgrid mtest_c)
  target_link_libraries(redscat2  simgrid mtest_c)
  target_link_libraries(redscat3  simgrid mtest_c)
  target_link_libraries(redscatbkinter  simgrid mtest_c)
  target_link_libraries(redscatblk3  simgrid mtest_c)
  target_link_libraries(red_scat_block2  simgrid mtest_c)
  target_link_libraries(red_scat_block  simgrid mtest_c)
  target_link_libraries(redscat  simgrid mtest_c)
#  target_link_libraries(redscatinter  simgrid mtest_c)
  target_link_libraries(reduce_mpich  simgrid mtest_c)
  target_link_libraries(reduce_local  simgrid mtest_c)
  target_link_libraries(scantst  simgrid mtest_c)
  target_link_libraries(scatter2  simgrid mtest_c)
  target_link_libraries(scatter3  simgrid mtest_c)
  target_link_libraries(scattern  simgrid mtest_c)
  target_link_libraries(scatterv  simgrid mtest_c)
#  target_link_libraries(uoplong  simgrid mtest_c)

  set_target_properties(allred PROPERTIES COMPILE_FLAGS "-O0" LINK_FLAGS "-O0")
endif()

set(examples_src
  ${examples_src}
 ${CMAKE_CURRENT_SOURCE_DIR}/allgather2.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/allgather3.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/allgatherv2.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/allgatherv3.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/allgatherv4.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/allgatherv4_manual.c 
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
 ${CMAKE_CURRENT_SOURCE_DIR}/bcast2.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/bcast3.c 
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
 ${CMAKE_CURRENT_SOURCE_DIR}/gather2_save.c 
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
 ${CMAKE_CURRENT_SOURCE_DIR}/nonblocking4.c
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
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE)
