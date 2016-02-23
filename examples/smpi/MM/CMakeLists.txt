if(enable_smpi)
  set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  if(NOT WIN32)
    add_executable(MM_mpi MM_mpi.c 2.5D_MM.c Summa.c Matrix_init.c)
    target_link_libraries(MM_mpi simgrid pthread)
  endif()
endif()

set(tesh_files
  ${tesh_files}
  PARENT_SCOPE)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/MM_mpi.c
  ${CMAKE_CURRENT_SOURCE_DIR}/2.5D_MM.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Summa.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Matrix_init.c
  ${CMAKE_CURRENT_SOURCE_DIR}/2.5D_MM.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Summa.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Matrix_init.h
  PARENT_SCOPE)
