foreach(x cmdline log_large log_usage)
  add_executable       (${x}  EXCLUDE_FROM_ALL ${x}/${x}.c)
  target_link_libraries(${x}  simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
  add_dependencies(tests ${x})

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)
endforeach()

foreach(x parallel_log_crashtest parmap_bench parmap_test signals)
  add_executable       (${x}  EXCLUDE_FROM_ALL ${x}/${x}.cpp)
  target_link_libraries(${x}  simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
  set_property(TARGET ${x} APPEND PROPERTY INCLUDE_DIRECTORIES "${INTERNAL_INCLUDES}")
  add_dependencies(tests ${x})

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.cpp)
endforeach()

set(tesh_files    ${tesh_files}     ${CMAKE_CURRENT_SOURCE_DIR}/log_usage/log_usage_ndebug.tesh    PARENT_SCOPE)
set(teshsuite_src ${teshsuite_src}                                                                 PARENT_SCOPE)

foreach(x cmdline log_large parallel_log_crashtest parmap_test)
  ADD_TESH(tesh-xbt-${x} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/xbt/${x} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/xbt/${x} ${x}.tesh)
endforeach()
if(enable_coverage)
  ADD_TESH(tesh-xbt-parmap_bench --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/xbt/parmap_bench --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/xbt/parmap_bench parmap_bench.tesh)
endif()

ADD_TESH(tesh-xbt-signals --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/xbt/signals --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/xbt/signals signals.tesh)

if(enable_debug)
  ADD_TESH(tesh-xbt-log --cd ${CMAKE_BINARY_DIR}/teshsuite/xbt/log_usage ${CMAKE_HOME_DIRECTORY}/teshsuite/xbt/log_usage/log_usage.tesh)
else()
  ADD_TESH(tesh-xbt-log --cd ${CMAKE_BINARY_DIR}/teshsuite/xbt/log_usage ${CMAKE_HOME_DIRECTORY}/teshsuite/xbt/log_usage/log_usage_ndebug.tesh)
endif()
