if(enable_smpi AND enable_smpi_MPICH3_testsuite)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

  foreach(file cmfree cmsplit2 cmsplit cmsplit_type commcreate1 comm_create_group comm_group_half comm_group_rand 
          comm_info ctxalloc ctxsplit dup dup_with_info) 
    # not compiled files
    # comm_idup comm_idup_mul comm_idup_overlap commname dupic ic1 ic2 iccreate icgroup icm icsplit probe-intercomm 
    add_executable(${file} ${file}.c)
    target_link_libraries(${file} simgrid mtest_c)
  endforeach()
endif()

foreach(file cmfree cmsplit2 cmsplit cmsplit_type commcreate1 comm_create_group comm_group_half comm_group_rand 
        comm_idup comm_idup_overlap comm_idup_mul comm_info commname ctxalloc ctxsplit dup dupic dup_with_info ic1 ic2 
        iccreate icgroup icm icsplit probe-intercomm)
  set(examples_src ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/${file}.c)
endforeach()

set(examples_src ${examples_src}                                         PARENT_SCOPE)
set(txt_files    ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/testlist   PARENT_SCOPE)
