foreach(x heap_bench log_large mallocator parallel_log_crashtest parmap_bench parmap_test)
    add_executable       (${x}  ${x}/${x}.c)
    target_link_libraries(${x}  simgrid)
    set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})

    set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
    set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)
endforeach()

foreach(x heap_bench log_large parallel_log_crashtest parmap_test) #mallocator parmap_bench
    ADD_TESH(tesh-xbt-${x} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/xbt/${x} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/xbt/${x} ${x}.tesh)
endforeach()

set(teshsuite_src ${teshsuite_src}                                             PARENT_SCOPE)
set(tesh_files    ${tesh_files}                                                PARENT_SCOPE)
