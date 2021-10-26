foreach(example actor-create actor-daemon actor-join actor-kill actor-migrate actor-suspend actor-yield actor-lifetime
                comm-wait comm-waitall comm-waitany
                exec-async exec-basic exec-dvfs exec-remote
                network-nonlinear clusters-multicpu io-degradation exec-cpu-nonlinear)
  set(tesh_files    ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}.tesh)
  set(examples_src  ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}.py)

  if(enable_python)
    ADD_TESH(python-${example}
                               --setenv srcdir=${CMAKE_CURRENT_SOURCE_DIR}/${example}
                               --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
                               --setenv pythoncmd=${PYTHON_EXECUTABLE}
                               --setenv LD_LIBRARY_PATH=${TESH_LIBRARY_PATH}
                               --setenv PYTHONPATH=${CMAKE_BINARY_DIR}/lib
                               --cd ${CMAKE_CURRENT_SOURCE_DIR}/${example}
                               ${example}.tesh)
  endif()
endforeach()

set(examples_src  ${examples_src}                                                      PARENT_SCOPE)
set(tesh_files    ${tesh_files}   examples/python/actor-create/actor-create_d.xml
                                  examples/python/actor-lifetime/actor-lifetime_d.xml
                                  examples/python/comm-wait/comm-wait_d.xml
                                  examples/python/comm-waitall/comm-waitall_d.xml
                                  examples/python/comm-waitany/comm-waitany_d.xml    PARENT_SCOPE)
