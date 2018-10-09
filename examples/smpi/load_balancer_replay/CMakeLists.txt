if(enable_smpi)
  set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  set(CMAKE_CXX_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicxx")
  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  add_executable       (load_balancer_replay load_balancer_replay.cpp)
  target_link_libraries(load_balancer_replay simgrid)
  #  ADD_TESH(sampi-load-balancer-replay --setenv srcdir=${CMAKE_CURRENT_SOURCE_DIR} --setenv bindir=${CMAKE_CURRENT_BINARY_DIR} --cd ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/load_balancer_replay.tesh)
endif()

# Uncomment this to install tesh files
#set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/load_balancer_replay.tesh           PARENT_SCOPE)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/load_balancer_replay.cpp            PARENT_SCOPE)
