set(example java_semaphore_gc)
set(sources ${CMAKE_CURRENT_SOURCE_DIR}/SemaphoreGC.java)

if(enable_java)
  add_custom_command(
    COMMENT "Building ${example}..."
    OUTPUT ${example}_compiled
    DEPENDS ${sources} simgrid-java_jar ${SIMGRID_JAR}
    COMMAND ${JAVA_COMPILE} -classpath ${SIMGRID_JAR} -d ${CMAKE_CURRENT_BINARY_DIR}/.. ${sources}
    COMMAND ${CMAKE_COMMAND} -E remove ${example}_compiled
    COMMAND ${CMAKE_COMMAND} -E touch ${example}_compiled
  )
  add_custom_target(${example} ALL DEPENDS ${example}_compiled)

  ADD_TESH(tesh-java-semaphore-gc --setenv srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/java --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/teshsuite/java ${CMAKE_HOME_DIRECTORY}/teshsuite/java/semaphore/semaphore_gc.tesh)
endif()

set(tesh_files   ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/semaphore_gc.tesh PARENT_SCOPE)
set(examples_src ${examples_src} ${sources}                                    PARENT_SCOPE)
