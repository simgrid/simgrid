set(semaphoregc_files  SemaphoreGC)
set(sleephostoff_files SleepHostOff)

foreach(example semaphoregc sleephostoff)
  foreach (filename ${${example}_files} )
    set(sources "${${example}_sources}" "${CMAKE_CURRENT_SOURCE_DIR}/${example}/${filename}.java")
  endforeach()

  set(tesh_files   ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}.tesh)

  if(enable_java)
    string (REPLACE "-" "/" example_dir ${example})
    add_jar(java-${example} SOURCES ${sources}
            INCLUDE_JARS simgrid-java_jar
            OUTPUT_NAME ${example}
            OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${example_dir})
    add_dependencies(java-all java-${example})

    ADD_TESH(tesh-java-${example}  --setenv javacmd=${Java_JAVA_EXECUTABLE} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv LD_LIBRARY_PATH=${TESH_LIBRARY_PATH} --setenv classpath=${TESH_CLASSPATH}${TESH_CLASSPATH_SEPARATOR}${example}.jar --cd ${CMAKE_BINARY_DIR}/teshsuite/java/${example_dir}  ${CMAKE_HOME_DIRECTORY}/teshsuite/java/${example_dir}/${example}.tesh)
  endif()
  set(examples_src ${examples_src} ${sources})
endforeach()

set(examples_src ${examples_src}                                                               PARENT_SCOPE)
set(tesh_files   ${tesh_files}                                                                 PARENT_SCOPE)
