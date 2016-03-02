if(enable_smpi)
  set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  if(NOT WIN32)
    add_executable       (MM_mpi MM_mpi.c Summa.c Matrix_init.c)
    target_link_libraries(MM_mpi simgrid)
  endif()
endif()

set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/MM_mpi.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Summa.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Matrix_init.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Summa.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Matrix_init.h
  PARENT_SCOPE)
