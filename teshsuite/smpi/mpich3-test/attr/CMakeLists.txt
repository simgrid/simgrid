if(enable_smpi AND enable_smpi_MPICH3_testsuite)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

  add_executable(attr2type attr2type.c)
  add_executable(attrend2 attrend2.c)
  add_executable(attrend attrend.c)
  add_executable(attrerr attrerr.c)
  add_executable(attrerrcomm attrerrcomm.c)
  add_executable(attrerrtype attrerrtype.c)
#  add_executable(attric attric.c)
  add_executable(attrorder attrorder.c)
  add_executable(attrordercomm attrordercomm.c)
  add_executable(attrordertype attrordertype.c)
  add_executable(attrt attrt.c)
  add_executable(baseattr2 baseattr2.c)
  add_executable(baseattrcomm baseattrcomm.c)
  add_executable(fkeyval fkeyval.c)
#  add_executable(fkeyvalcomm fkeyvalcomm.c)
#  add_executable(fkeyvaltype fkeyvaltype.c)
#  add_executable(keyval_double_free keyval_double_free.c)

  target_link_libraries(attr2type simgrid mtest_c)
  target_link_libraries(attrend2 simgrid mtest_c)
  target_link_libraries(attrend simgrid mtest_c)
  target_link_libraries(attrerr simgrid mtest_c)
  target_link_libraries(attrerrcomm simgrid mtest_c)
  target_link_libraries(attrerrtype simgrid mtest_c)
#  target_link_libraries(attric simgrid mtest_c)
  target_link_libraries(attrorder simgrid mtest_c)
  target_link_libraries(attrordercomm simgrid mtest_c)
  target_link_libraries(attrordertype simgrid mtest_c)
  target_link_libraries(attrt simgrid mtest_c)
  target_link_libraries(baseattr2 simgrid mtest_c)
  target_link_libraries(baseattrcomm simgrid mtest_c)
  target_link_libraries(fkeyval simgrid mtest_c)
#  target_link_libraries(fkeyvalcomm simgrid mtest_c)
#  target_link_libraries(fkeyvaltype simgrid mtest_c)
#  target_link_libraries(keyval_double_free simgrid mtest_c)

endif()

set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/attr2type.c
  ${CMAKE_CURRENT_SOURCE_DIR}/attrend2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/attrend.c
  ${CMAKE_CURRENT_SOURCE_DIR}/attrerr.c
  ${CMAKE_CURRENT_SOURCE_DIR}/attrerrcomm.c
  ${CMAKE_CURRENT_SOURCE_DIR}/attrerrtype.c
  ${CMAKE_CURRENT_SOURCE_DIR}/attric.c
  ${CMAKE_CURRENT_SOURCE_DIR}/attrorder.c
  ${CMAKE_CURRENT_SOURCE_DIR}/attrordercomm.c
  ${CMAKE_CURRENT_SOURCE_DIR}/attrordertype.c
  ${CMAKE_CURRENT_SOURCE_DIR}/attrt.c
  ${CMAKE_CURRENT_SOURCE_DIR}/baseattr2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/baseattrcomm.c
  ${CMAKE_CURRENT_SOURCE_DIR}/fkeyval.c
  ${CMAKE_CURRENT_SOURCE_DIR}/fkeyvalcomm.c
  ${CMAKE_CURRENT_SOURCE_DIR}/fkeyvaltype.c
  ${CMAKE_CURRENT_SOURCE_DIR}/keyval_double_free.c
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE)
