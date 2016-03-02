if(enable_smpi)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  endif()
  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  add_executable       (vector_test vector_test.c)
  target_link_libraries(vector_test simgrid)

  ADD_TESH_FACTORIES(tesh-smpi-vector "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/vector --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/vector vector.tesh)
endif()

set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/vector.tesh    PARENT_SCOPE)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/vector_test.c  PARENT_SCOPE)
