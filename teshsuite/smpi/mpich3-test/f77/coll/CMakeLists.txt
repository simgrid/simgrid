cmake_minimum_required(VERSION 2.6)

if(enable_smpi)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
  set(MPICH_FLAGS "-DHAVE_STDLIB_H=1 -DHAVE_UNISTD_H=1 -DHAVE_STRING_H=1 -DUSE_STDARG=1 -DHAVE_LONG_DOUBLE=1 -DHAVE_PROTOTYPES=1 -DHAVE_SIGNAL_H=1 -DHAVE_SIGACTION=1 -DHAVE_SLEEP=1 -DHAVE_SYSCONF=1  -Wno-error=unused-variable -Wno-implicit -g")
  include_directories("${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_BINARY_DIR}/../include/")


  add_executable(allredint8f allredint8f.f ../util/mtestf.f)
  add_executable(allredopttf allredopttf.f ../util/mtestf.f)
  add_executable(alltoallvf alltoallvf.f ../util/mtestf.f)
  add_executable(alltoallwf alltoallwf.f ../util/mtestf.f)
  add_executable(exscanf exscanf.f ../util/mtestf.f)
  add_executable(inplacef inplacef.f ../util/mtestf.f)
 # add_executable(nonblockingf nonblockingf.f ../util/mtestf.f)
 # add_executable(nonblocking_inpf nonblocking_inpf.f ../util/mtestf.f)
  add_executable(red_scat_blockf red_scat_blockf.f ../util/mtestf.f)
  add_executable(redscatf redscatf.f ../util/mtestf.f)
  add_executable(reducelocalf reducelocalf.f ../util/mtestf.f)
  add_executable(split_typef split_typef.f ../util/mtestf.f)
  add_executable(uallreducef uallreducef.f ../util/mtestf.f)
  add_executable(vw_inplacef vw_inplacef.f ../util/mtestf.f)



  target_link_libraries(allredint8f  simgrid)
  target_link_libraries(allredopttf  simgrid)
  target_link_libraries(alltoallvf  simgrid)
  target_link_libraries(alltoallwf  simgrid)
  target_link_libraries(exscanf  simgrid)
  target_link_libraries(inplacef  simgrid)
 # target_link_libraries(nonblockingf  simgrid)
 # target_link_libraries(nonblocking_inpf  simgrid)
  target_link_libraries(red_scat_blockf  simgrid)
  target_link_libraries(redscatf  simgrid)
  target_link_libraries(reducelocalf  simgrid)
  target_link_libraries(split_typef  simgrid)
  target_link_libraries(uallreducef  simgrid)
  target_link_libraries(vw_inplacef  simgrid)



 set_target_properties(allredint8f PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(allredopttf PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(alltoallvf PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(alltoallwf PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(exscanf PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(inplacef PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(nonblockingf PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(nonblocking_inpf PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(red_scat_blockf PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(redscatf PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(reducelocalf PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(split_typef PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(uallreducef PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(vw_inplacef PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")

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
 ${CMAKE_CURRENT_SOURCE_DIR}/allredint8f.f 
 ${CMAKE_CURRENT_SOURCE_DIR}/allredopttf.f 
 ${CMAKE_CURRENT_SOURCE_DIR}/alltoallvf.f 
 ${CMAKE_CURRENT_SOURCE_DIR}/alltoallwf.f 
 ${CMAKE_CURRENT_SOURCE_DIR}/exscanf.f 
 ${CMAKE_CURRENT_SOURCE_DIR}/inplacef.f 
 ${CMAKE_CURRENT_SOURCE_DIR}/nonblockingf.f 
 ${CMAKE_CURRENT_SOURCE_DIR}/nonblocking_inpf.f 
 ${CMAKE_CURRENT_SOURCE_DIR}/red_scat_blockf.f 
 ${CMAKE_CURRENT_SOURCE_DIR}/redscatf.f 
 ${CMAKE_CURRENT_SOURCE_DIR}/reducelocalf.f 
 ${CMAKE_CURRENT_SOURCE_DIR}/split_typef.f 
 ${CMAKE_CURRENT_SOURCE_DIR}/uallreducef.f 
 ${CMAKE_CURRENT_SOURCE_DIR}/vw_inplacef.f 
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/runtests
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE
  )
