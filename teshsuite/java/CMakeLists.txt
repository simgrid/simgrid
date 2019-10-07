set(semaphoregc_files  SemaphoreGC)
set(sleephostoff_files SleepHostOff)

foreach(example semaphoregc sleephostoff)
  foreach (filename ${${example}_files} )
    set(sources "${${example}_sources}" "${CMAKE_CURRENT_SOURCE_DIR}/${example}/${filename}.java")
  endforeach()

  set(tesh_files   ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}.tesh)

  if(enable_java)
    add_custom_command(
      COMMENT "Building ${example}..."
      OUTPUT  java-${example}_compiled
      DEPENDS ${sources} simgrid-java_jar ${SIMGRID_JAR}
      COMMAND ${JAVA_COMPILE} -classpath ${SIMGRID_JAR} -d ${CMAKE_CURRENT_BINARY_DIR} ${sources}
      COMMAND ${CMAKE_COMMAND} -E remove ${example}/java-${example}_compiled
      COMMAND ${CMAKE_COMMAND} -E touch ${example}/java-${example}_compiled
    )
    add_custom_target(java-${example} ALL DEPENDS java-${example}_compiled)
    add_dependencies(java-all java-${example})
    set_target_properties(java-${example}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${example})
  endif()
  set(examples_src ${examples_src} ${sources})
endforeach()

set(examples_src ${examples_src}                                                               PARENT_SCOPE)
set(tesh_files   ${tesh_files}                                                                 PARENT_SCOPE)

if(enable_java)
  ADD_TESH(tesh-java-semaphoregc  --setenv javacmd=${Java_JAVA_EXECUTABLE} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv LD_LIBRARY_PATH=${TESH_LIBRARY_PATH} --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/teshsuite/java/semaphoregc  ${CMAKE_HOME_DIRECTORY}/teshsuite/java/semaphoregc/semaphoregc.tesh)
  ADD_TESH(tesh-java-sleephostoff --setenv javacmd=${Java_JAVA_EXECUTABLE} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv LD_LIBRARY_PATH=${TESH_LIBRARY_PATH} --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/teshsuite/java/sleephostoff ${CMAKE_HOME_DIRECTORY}/teshsuite/java/sleephostoff/sleephostoff.tesh)
endif()
