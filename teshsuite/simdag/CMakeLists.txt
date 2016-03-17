foreach(x availability basic0 basic1 basic2 basic3 basic4 basic5 basic6 basic-link-test basic-parsing-test
          comm-mxn-all2all comm-mxn-independent comm-mxn-scatter comm-p2p-latency-1 
          comm-p2p-latency-2 comm-p2p-latency-3 comm-p2p-latency-bound comp-only-par comp-only-seq incomplete)
  add_executable       (${x}  ${x}/${x}.c)
  target_link_libraries(${x}  simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)

  ADD_TESH(tesh-simdag-${x} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simdag/${x} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/${x} ${x}.tesh)
endforeach()

foreach(x flatifier is-router)
  add_executable       (${x}  ${x}/${x}.cpp)
  target_link_libraries(${x}  simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.cpp)

  ADD_TESH(tesh-simdag-${x} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simdag/${x} --setenv srcdir=${CMAKE_HOME_DIRECTORY} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/${x} ${x}.tesh)
endforeach()

ADD_TESH(tesh-simdag-parser-bypass   --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simdag/basic-parsing-test --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/basic-parsing-test --setenv srcdir=${CMAKE_HOME_DIRECTORY} basic-parsing-test-bypass.tesh)
ADD_TESH(tesh-simdag-parser-sym-full --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simdag/basic-parsing-test --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/basic-parsing-test basic-parsing-test-sym-full.tesh)

ADD_TEST(tesh-simdag-full-links01    ${CMAKE_BINARY_DIR}/teshsuite/simdag/basic-parsing-test/basic-parsing-test ${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/platforms/two_clusters.xml FULL_LINK)
ADD_TEST(tesh-simdag-full-links02    ${CMAKE_BINARY_DIR}/teshsuite/simdag/basic-parsing-test/basic-parsing-test ${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/platforms/two_clusters_one_name.xml FULL_LINK)
ADD_TEST(tesh-simdag-one-link-g5k    ${CMAKE_BINARY_DIR}/teshsuite/simdag/basic-parsing-test/basic-parsing-test ${CMAKE_HOME_DIRECTORY}/examples/platforms/g5k.xml ONE_LINK)

set(teshsuite_src ${teshsuite_src}  PARENT_SCOPE)
set(tesh_files    ${tesh_files}     ${CMAKE_CURRENT_SOURCE_DIR}/basic-parsing-test/basic-parsing-test-sym-full.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/basic-parsing-test/basic-parsing-test-bypass.tesh
                                    PARENT_SCOPE)
