set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Regular pthread examples: test the interceptor of the pthread interface
#########################################################################

foreach(x
        cond-timedwait_ok exit
        mutex-simple mutex-recursive
      	producer-consumer)

  if(enable_sthread)
    add_executable       (pthread-${x} EXCLUDE_FROM_ALL pthread-${x}.c)
    set_target_properties(pthread-${x} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(pthread-${x} PRIVATE Threads::Threads)

    add_dependencies(tests pthread-${x})
    ADD_TESH(pthread-${x} --setenv libdir=${CMAKE_BINARY_DIR}/lib --cd ${CMAKE_BINARY_DIR}/examples/sthread ${CMAKE_CURRENT_SOURCE_DIR}/pthread-${x}.tesh)

    if(SIMGRID_HAVE_MC)
      add_dependencies(tests-mc pthread-${x})
      ADD_TESH(pthread-mc-${x} --setenv libdir=${CMAKE_BINARY_DIR}/lib --cd ${CMAKE_BINARY_DIR}/examples/sthread ${CMAKE_CURRENT_SOURCE_DIR}/pthread-mc-${x}.tesh)
    endif()
  endif()

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/pthread-${x}.tesh
                                     ${CMAKE_CURRENT_SOURCE_DIR}/pthread-mc-${x}.tesh)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/pthread-${x}.c)

endforeach()

# Regular pthread examples that may deadlock: test sthread + MC in that case
############################################################################

foreach(x
        mutex-simpledeadlock)

  if(enable_sthread)

    add_executable       (pthread-${x} EXCLUDE_FROM_ALL pthread-${x}.c)
    set_target_properties(pthread-${x} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(pthread-${x} PRIVATE Threads::Threads)

    if(SIMGRID_HAVE_MC)
      add_dependencies(tests-mc pthread-${x})
      ADD_TESH(pthread-mc-${x} --setenv libdir=${CMAKE_BINARY_DIR}/lib --cd ${CMAKE_BINARY_DIR}/examples/sthread ${CMAKE_CURRENT_SOURCE_DIR}/pthread-mc-${x}.tesh)
    endif()
  endif()

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/pthread-mc-${x}.tesh)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/pthread-${x}.c)

endforeach()

# MPI + pthread tests in C (with or without error)
###################################################

set(_mutex-simpledeadlock_failing ON)

foreach(x
        mutex-simple mutex-simpledeadlock)

  if(enable_sthread AND enable_smpi)
    add_executable       (smpi-${x} EXCLUDE_FROM_ALL smpi-${x}.c)
    set_target_properties(smpi-${x} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(smpi-${x} PRIVATE Threads::Threads)
    smpi_c_target(smpi-${x})

    add_dependencies(tests smpi-${x})
    if(NOT DEFINED _${x}_failing)
      ADD_TESH(smpi-${x} --setenv libdir=${CMAKE_BINARY_DIR}/lib --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_BINARY_DIR}/examples/sthread ${CMAKE_CURRENT_SOURCE_DIR}/smpi-${x}.tesh)
    endif()
    
    if(SIMGRID_HAVE_MC)
      add_dependencies(tests-mc smpi-${x})
      ADD_TESH_FACTORIES(smpi-mc-${x} "^thread" --setenv libdir=${CMAKE_BINARY_DIR}/lib --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_BINARY_DIR}/examples/sthread --setenv "extraparam=--cfg=model-check/no-fork:off" ${CMAKE_CURRENT_SOURCE_DIR}/smpi-mc-${x}.tesh)
      ADD_TESH_FACTORIES(smpi-mc-${x}  "thread" --setenv libdir=${CMAKE_BINARY_DIR}/lib --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_BINARY_DIR}/examples/sthread --setenv "extraparam=--cfg=model-check/no-fork:on"  ${CMAKE_CURRENT_SOURCE_DIR}/smpi-mc-${x}.tesh)
    endif()
  endif()
  
  if(NOT DEFINED _${x}_failing)
    set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/smpi-${x}.tesh)
  endif()
  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/smpi-mc-${x}.tesh)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/smpi-${x}.c)
  unset(_${x}_failing)

endforeach()

# C++ tests with sthread+MC that live in their own directory
########################################################
foreach(example
        stdobject)

  if(${enable_sthread})

    add_executable       (${example} EXCLUDE_FROM_ALL ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}.cpp)
    set_target_properties(${example} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(${example} PRIVATE Threads::Threads)
    set_target_properties(${example} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${example})

    if(SIMGRID_HAVE_MC)
      add_dependencies(tests-mc ${example})
      ADD_TESH_FACTORIES(sthread-mc-${example} "^thread" --setenv libdir=${CMAKE_BINARY_DIR}/lib --cd ${CMAKE_BINARY_DIR}/examples/sthread/${example} --setenv "extraparam=--cfg=model-check/no-fork:off" ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}.tesh)
      ADD_TESH_FACTORIES(sthread-mc-${example}  "thread" --setenv libdir=${CMAKE_BINARY_DIR}/lib --cd ${CMAKE_BINARY_DIR}/examples/sthread/${example} --setenv "extraparam=--cfg=model-check/no-fork:on"  ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}.tesh)
    endif()
  endif()

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}.tesh)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}.cpp)
endforeach()

# Add all extra files to the archive
####################################

set(tesh_files    ${tesh_files}     PARENT_SCOPE)
set(bin_files     ${bin_files}      PARENT_SCOPE)
set(examples_src  ${examples_src}   PARENT_SCOPE)
set(xml_files     ${xml_files}      PARENT_SCOPE)


