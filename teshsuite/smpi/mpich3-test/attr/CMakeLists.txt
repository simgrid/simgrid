cmake_minimum_required(VERSION 2.6)

if(enable_smpi)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

  include_directories("${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

  add_executable(attr2type attr2type.c ../util/mtest.c)
  add_executable(attrend2 attrend2.c ../util/mtest.c)
  add_executable(attrend attrend.c ../util/mtest.c)
  add_executable(attrerr attrerr.c ../util/mtest.c)
  add_executable(attrerrcomm attrerrcomm.c ../util/mtest.c)
  add_executable(attrerrtype attrerrtype.c ../util/mtest.c)
  add_executable(attric attric.c ../util/mtest.c)
  add_executable(attrorder attrorder.c ../util/mtest.c)
  add_executable(attrordercomm attrordercomm.c ../util/mtest.c)
  add_executable(attrordertype attrordertype.c ../util/mtest.c)
  add_executable(attrt attrt.c ../util/mtest.c)
  add_executable(baseattr2 baseattr2.c ../util/mtest.c)
  add_executable(baseattrcomm baseattrcomm.c ../util/mtest.c)
  add_executable(fkeyval fkeyval.c ../util/mtest.c)
  add_executable(fkeyvalcomm fkeyvalcomm.c ../util/mtest.c)
  add_executable(fkeyvaltype fkeyvaltype.c ../util/mtest.c)
  add_executable(keyval_double_free keyval_double_free.c ../util/mtest.c)

  target_link_libraries(attr2type  simgrid)
  target_link_libraries(attrend2 simgrid)
  target_link_libraries(attrend simgrid)
  target_link_libraries(attrerr simgrid)
  target_link_libraries(attrerrcomm simgrid)
  target_link_libraries(attrerrtype simgrid)
  target_link_libraries(attric simgrid)
  target_link_libraries(attrorder simgrid)
  target_link_libraries(attrordercomm simgrid)
  target_link_libraries(attrordertype simgrid)
  target_link_libraries(attrt simgrid)
  target_link_libraries(baseattr2 simgrid)
  target_link_libraries(baseattrcomm simgrid)
  target_link_libraries(fkeyval simgrid)
  target_link_libraries(fkeyvalcomm simgrid)
  target_link_libraries(fkeyvaltype simgrid)
  target_link_libraries(keyval_double_free simgrid)

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
