foreach(example SemaphoreGC SleepHostOff)
  set(sources ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}.java)
  set(tesh_files   ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}.tesh)

  if(enable_java)
    add_custom_command(
      COMMENT "Building ${example}..."
      OUTPUT  ${example}_compiled
      DEPENDS ${sources} simgrid-java_jar ${SIMGRID_JAR}
      COMMAND ${JAVA_COMPILE} -classpath ${SIMGRID_JAR} -d ${CMAKE_CURRENT_BINARY_DIR} ${sources}
      COMMAND ${CMAKE_COMMAND} -E remove ${example}/${example}_compiled
      COMMAND ${CMAKE_COMMAND} -E touch ${example}/${example}_compiled
    )
    add_custom_target(${example} ALL DEPENDS ${example}_compiled)
    set_target_properties(${example}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${example})

    ADD_TESH(tesh-java-${example} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/java --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/teshsuite/java ${CMAKE_HOME_DIRECTORY}/teshsuite/java/${example}/${example}.tesh)
  endif()
  set(examples_src ${examples_src} ${sources})
endforeach()

set(examples_src ${examples_src} ${sources}                                                    PARENT_SCOPE)
set(tesh_files   ${tesh_files}                                                                 PARENT_SCOPE)
