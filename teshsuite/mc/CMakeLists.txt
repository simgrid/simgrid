
foreach(x random-bug mutex-handling)

  if(NOT DEFINED ${x}_sources)
    set(${x}_sources ${x}/${x}.cpp)
  endif()

  if(NOT DEFINED ${x}_disable)
    add_executable       (${x}  EXCLUDE_FROM_ALL ${${x}_sources})
    target_link_libraries(${x}  simgrid)
    set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
    set_property(TARGET ${x} APPEND PROPERTY INCLUDE_DIRECTORIES "${INTERNAL_INCLUDES}")
    target_compile_options(${x} PRIVATE ${CMAKE_C_DEBUG_FLAGS})
    add_dependencies(tests-mc ${x})
  endif()

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)

  foreach(file ${${x}_sources})
    set(teshsuite_src  ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${file})
  endforeach()

  unset(${x}_sources)
  unset(${x}_disable)

endforeach()

add_executable       (without-mutex-handling EXCLUDE_FROM_ALL mutex-handling/mutex-handling.cpp)
target_link_libraries(without-mutex-handling simgrid)
set_target_properties(without-mutex-handling PROPERTIES COMPILE_FLAGS -DDISABLE_THE_MUTEX=1)
set_target_properties(without-mutex-handling PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/mutex-handling)
add_dependencies(tests-mc without-mutex-handling)

ADD_TESH(mc-random-bug-replay                --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc/random-bug --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc/random-bug random-bug-replay.tesh)
IF(SIMGRID_HAVE_MC)
  ADD_TESH(tesh-mc-mutex-handling-dpor         --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc/mutex-handling --setenv srcdir=${CMAKE_HOME_DIRECTORY} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc/mutex-handling mutex-handling.tesh --cfg=model-check/reduction:dpor)
  ADD_TESH(tesh-mc-without-mutex-handling-dpor --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc/mutex-handling --setenv srcdir=${CMAKE_HOME_DIRECTORY} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc/mutex-handling without-mutex-handling.tesh --cfg=model-check/reduction:dpor)
  ADD_TESH(mc-random-bug                       --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc/random-bug --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc/random-bug random-bug.tesh)
ENDIF()

if(enable_coverage)
  ADD_TEST(cover-mc-mutex-handling ${CMAKE_CURRENT_BINARY_DIR}/mutex-handling/mutex-handling ${CMAKE_HOME_DIRECTORY}/examples/platforms/small_platform.xml)
endif()

foreach(x
            simple_barrier_ok              simple_barrier_deadlock
            simple_barrier_with_threads_ok simple_barrier_with_threads_deadlock

            simple_cond_ok                 simple_cond_deadlock
            simple_cond_broadcast_ok       simple_cond_broadcast_deadlock

#            simple_cond_broadcast_with_semaphore_ok 
      	     simple_cond_broadcast_with_semaphore_deadlock1  simple_cond_broadcast_with_semaphore_deadlock2

             simple_mutex_ok simple_mutex_deadlock
             simple_mutex_with_threads_ok simple_mutex_with_threads_deadlock

             simple_semaphore_deadlock simple_semaphores_deadlock
             simple_semaphores_ok
             simple_semaphores_with_threads_ok simple_semaphores_with_threads_deadlock
	     
             simple_threads_ok

             barber_shop_ok             barber_shop_deadlock
             philosophers_semaphores_ok philosophers_semaphores_deadlock
             philosophers_mutex_ok      philosophers_mutex_deadlock
             producer_consumer_ok       producer_consumer_deadlock    
             
             # philosophers_spurious_deadlock # infinite no-op loop             
             # producer_consumer_spurious_nok # infinite no-op loop
             )

  set(teshsuite_src  ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/mcmini/${x}.c)
  set(tesh_files     ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/mcmini/${x}.tesh)

  if(${enable_sthread} AND ${enable_testsuite_McMini} AND SIMGRID_HAVE_MC)
    add_executable       (mcmini-${x}  EXCLUDE_FROM_ALL mcmini/${x}.c)         
    set_target_properties(mcmini-${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/mcmini)
    target_link_libraries(mcmini-${x}  PRIVATE Threads::Threads)
    add_dependencies(tests-mc mcmini-${x})

    ADD_TESH(mc-mini-${x} --setenv libdir=${CMAKE_BINARY_DIR}/lib --setenv bindir=${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/mcmini/${x}.tesh)
  endif()
endforeach()
if(${enable_testsuite_McMini} AND (NOT ${enable_sthread}))
  message(STATUS "McMini tests cannot be executed when sthread is disabled.")
endif()
if(${enable_testsuite_McMini} AND (NOT SIMGRID_HAVE_MC))
  message(STATUS "McMini tests cannot be executed when MC is disabled.")
endif()

set(probe_deadlock_disable 1)
set(probe_ok_disable 1)
foreach(x iprobe_ok iprobe_nok probe_deadlock probe_ok)
  if(NOT DEFINED ${x}_disable)
    set(${x}_disable 0)
  endif()
  if(SIMGRID_HAVE_MC AND enable_smpi AND NOT ${${x}_disable})
    add_executable       (mpi_${x}  EXCLUDE_FROM_ALL mpi/${x}.c)
    smpi_c_target(mpi_${x})
    set_target_properties(mpi_${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_dependencies(tests-mc mpi_${x})

    ADD_TESH(mc-mpi-${x} --setenv libdir=${CMAKE_BINARY_DIR}/lib --setenv srcdir=${CMAKE_CURRENT_SOURCE_DIR} --setenv bindir=${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/mpi/${x}.tesh)
  endif()
  unset(${x}_disable)

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/mpi/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/mpi/${x}.c)
endforeach()


set(teshsuite_src  ${teshsuite_src}                                                                        PARENT_SCOPE)
set(tesh_files     ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/random-bug/random-bug-replay.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/mutex-handling/without-mutex-handling.tesh PARENT_SCOPE)

