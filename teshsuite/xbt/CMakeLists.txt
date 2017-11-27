foreach(x log_large log_usage mallocator parallel_log_crashtest)
  add_executable       (${x}  ${x}/${x}.c)
  target_link_libraries(${x}  simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)
endforeach()

foreach(x parmap_bench parmap_test)
  add_executable       (${x}  ${x}/${x}.cpp)
  target_link_libraries(${x}  simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.cpp)
endforeach()

if(HAVE_MMALLOC)
  add_executable       (mmalloc_test ${CMAKE_CURRENT_SOURCE_DIR}/mmalloc/mmalloc_test.cpp)
  target_link_libraries(mmalloc_test simgrid)
  set_target_properties(mmalloc_test  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/mmalloc)
endif()

set(tesh_files    ${tesh_files}     ${CMAKE_CURRENT_SOURCE_DIR}/log_usage/log_usage_ndebug.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/mmalloc/mmalloc_64.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/mmalloc/mmalloc_32.tesh          PARENT_SCOPE)
set(teshsuite_src ${teshsuite_src}  ${CMAKE_CURRENT_SOURCE_DIR}/mmalloc/mmalloc_test.cpp           PARENT_SCOPE)

foreach(x log_large parallel_log_crashtest parmap_test) #mallocator parmap_bench
  ADD_TESH(tesh-xbt-${x} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/xbt/${x} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/xbt/${x} ${x}.tesh)
endforeach()

if(enable_debug)
  ADD_TESH(tesh-xbt-log --cd ${CMAKE_BINARY_DIR}/teshsuite/xbt/log_usage ${CMAKE_HOME_DIRECTORY}/teshsuite/xbt/log_usage/log_usage.tesh)
else()
  ADD_TESH(tesh-xbt-log --cd ${CMAKE_BINARY_DIR}/teshsuite/xbt/log_usage ${CMAKE_HOME_DIRECTORY}/teshsuite/xbt/log_usage/log_usage_ndebug.tesh)
endif()

if(HAVE_MMALLOC)
  if(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32 bits
    ADD_TESH(tesh-xbt-mmalloc-32 --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/xbt/mmalloc --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/xbt/mmalloc mmalloc_32.tesh)
  else()
    ADD_TESH(tesh-xbt-mmalloc-64 --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/xbt/mmalloc --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/xbt/mmalloc mmalloc_64.tesh)
  endif()
endif()
