if(enable_smpi AND SMPI_FORTRAN)
  set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  add_executable       (sef sef.f)
  target_link_libraries(sef simgrid)
  ADD_TESH_FACTORIES(smpi-energy-f77 "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/smpi/energy --cd ${CMAKE_BINARY_DIR}/examples/smpi/energy ${CMAKE_HOME_DIRECTORY}/examples/smpi/energy/f77/energy.tesh)
endif()

set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/energy.tesh  PARENT_SCOPE)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/sef.f        PARENT_SCOPE)
