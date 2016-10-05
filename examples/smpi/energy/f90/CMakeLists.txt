if(enable_smpi AND SMPI_FORTRAN)
  set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpif90")
  add_executable       (sef90 sef90.f90)
  target_link_libraries(sef90 simgrid)
  ADD_TESH_FACTORIES(smpi-energy-f90 "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/smpi/energy --cd ${CMAKE_BINARY_DIR}/examples/smpi/energy ${CMAKE_HOME_DIRECTORY}/examples/smpi/energy/f90/energy.tesh)
endif()

set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/energy.tesh  PARENT_SCOPE)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/sef90.f90    PARENT_SCOPE)
