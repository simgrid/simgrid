if(enable_smpi)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  foreach(x type-hvector type-struct type-vector)
    add_executable       (${x}  ${x}/${x}.c)
    target_link_libraries(${x}  simgrid)
    set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})

    set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
    set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)

    ADD_TESH_FACTORIES(tesh-smpi-${x} "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/${x} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/${x} ${x}.tesh)
  endforeach()
endif()

set(bin_files       ${bin_files}      ${CMAKE_CURRENT_SOURCE_DIR}/hostfile
                                      ${CMAKE_CURRENT_SOURCE_DIR}/hostfile_coll  PARENT_SCOPE)
set(teshsuite_src ${teshsuite_src}                                             PARENT_SCOPE)
set(tesh_files    ${tesh_files}                                                PARENT_SCOPE)
