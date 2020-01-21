foreach(example actor-create actor-daemon actor-join actor-kill actor-migrate actor-suspend actor-yield # actor-lifetime
                async-wait async-waitall async-waitany
                exec-async exec-basic exec-dvfs exec-remote)
  set(tesh_files    ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}.tesh)
  set(examples_src  ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}.py)

  if(enable_python)
    ADD_TESH_FACTORIES(python-${example} "thread;ucontext;raw;boost"
                               --setenv srcdir=${CMAKE_CURRENT_SOURCE_DIR}/${example}
                               --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
                               --setenv pythoncmd=${PYTHON_EXECUTABLE}
			       --setenv LD_LIBRARY_PATH=${TESH_LIBRARY_PATH}
			       --setenv PYTHONPATH=${CMAKE_BINARY_DIR}/lib
			       --cd ${CMAKE_CURRENT_SOURCE_DIR}/${example}
                               ${example}.tesh)
  endif()
endforeach()

set(tesh_files    examples/python/actor-create/actor-create_d.xml
                  examples/python/actor-lifetime/actor-lifetime_d.xml
		  examples/python/actor-lifetime/actor-lifetime.py     # example broken so far
		  examples/python/actor-lifetime/actor-lifetime.tesh
		  examples/python/async-wait/async-wait_d.xml
		  examples/python/async-waitall/async-waitall_d.xml
		  examples/python/async-waitany/async-waitany_d.xml
                  ${tesh_files}    PARENT_SCOPE)
set(examples_src  ${examples_src}  PARENT_SCOPE)
