set(process_kill_sources      ${CMAKE_CURRENT_SOURCE_DIR}/process/kill/Main.java
                              ${CMAKE_CURRENT_SOURCE_DIR}/process/kill/Killer.java
                              ${CMAKE_CURRENT_SOURCE_DIR}/process/kill/Victim.java)
set(process_migration_sources ${CMAKE_CURRENT_SOURCE_DIR}/process/migration/Main.java
                              ${CMAKE_CURRENT_SOURCE_DIR}/process/migration/Emigrant.java
                              ${CMAKE_CURRENT_SOURCE_DIR}/process/migration/Policeman.java)
set(process_suspend_sources   ${CMAKE_CURRENT_SOURCE_DIR}/process/suspend/Main.java
                              ${CMAKE_CURRENT_SOURCE_DIR}/process/suspend/DreamMaster.java
                              ${CMAKE_CURRENT_SOURCE_DIR}/process/suspend/LazyGuy.java)

foreach (example process_kill process_migration process_suspend)
  if(enable_java)
    string (REPLACE "_" "/" example_dir ${example})

    add_custom_command(
      COMMENT "Building java_${example}..."
      OUTPUT ${example_dir}/java_${example}_compiled
      DEPENDS ${example_sources} simgrid-java_jar ${SIMGRID_JAR}
      COMMAND ${JAVA_COMPILE} -classpath ${SIMGRID_JAR} -d ${CMAKE_CURRENT_SOURCE_DIR} ${${example}_sources}
      COMMAND ${CMAKE_COMMAND} -E remove ${example_dir}/java_${example}_compiled
      COMMAND ${CMAKE_COMMAND} -E touch ${example_dir}/java_${example}_compiled
    )
    add_custom_target(${example} ALL DEPENDS ${example_dir}/java_${example}_compiled)
  endif()
  set(examples_src  ${examples_src}  ${${example}_sources})
  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${example_dir}/${example}.tesh)
endforeach()

set(examples_src  ${examples_src}  PARENT_SCOPE)
set(tesh_files    ${tesh_files}    PARENT_SCOPE)

if(enable_java)
  foreach (example process_kill process_migration process_suspend)
    string (REPLACE "_" "/" example_dir ${example})
    ADD_TESH(java-${example}  --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/java --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/examples/java ${CMAKE_HOME_DIRECTORY}/examples/java/${example_dir}/${example}.tesh)
  endforeach()
endif()

