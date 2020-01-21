foreach(example borken-context corrupt-stack)
  set(tesh_files    ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}_d.xml)
  set(examples_src  ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}.py)

  if(enable_python)
    ADD_TESH_FACTORIES(tesh-python-${example} "thread;ucontext;raw;boost"
                               --setenv srcdir=${CMAKE_CURRENT_SOURCE_DIR}/${example}
                               --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
                               --setenv pythoncmd=${PYTHON_EXECUTABLE}
			       --setenv LD_LIBRARY_PATH=${TESH_LIBRARY_PATH}
			       --setenv PYTHONPATH=${CMAKE_BINARY_DIR}/lib
			       --cd ${CMAKE_CURRENT_SOURCE_DIR}/${example}
                               ${example}.tesh)
  endif()
endforeach()

set(tesh_files    ${tesh_files}    PARENT_SCOPE)
set(examples_src  ${examples_src}  PARENT_SCOPE)
