set(JAVA_FILES ${JMSG_JAVA_SRC})
set(JAVA_CLASSES ${JAVA_FILES})

string(REPLACE "src/java/" "${PROJECT_DIRECTORY}/src/java/"
               JAVA_FILES "${JAVA_FILES}")

string(REPLACE "src/java/simgrid/msg" "${CMAKE_CURRENT_BINARY_DIR}/classes/simgrid/msg"
               JAVA_CLASSES "${JAVA_CLASSES}")
string(REPLACE ".java" ".class;" 
               JAVA_CLASSES "${JAVA_CLASSES}")
	       

add_custom_command(
  OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/classes/
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/classes/")
  
ADD_CUSTOM_COMMAND(
  TARGET clean
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/classes/)

# compile all .java files with javac to .class
ADD_CUSTOM_COMMAND(
  OUTPUT ${JAVA_CLASSES}
  DEPENDS ${JAVA_FILES} ${CMAKE_CURRENT_BINARY_DIR}/classes/
  COMMAND ${JAVA_COMPILE} -d ${CMAKE_CURRENT_BINARY_DIR}/classes/
                          -cp ${CMAKE_CURRENT_BINARY_DIR}/classes/
			  ${JAVA_FILES}
  COMMENT "Compiling java sources of core library..."
)

ADD_CUSTOM_TARGET(simgrid_java ALL
                  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/simgrid.jar simgrid)

ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/simgrid.jar
  DEPENDS ${JAVA_CLASSES}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/classes  
  COMMAND ${JAVA_ARCHIVE} -cvf ${CMAKE_CURRENT_BINARY_DIR}/simgrid.jar .
  COMMENT "Building simgrid.jar..."
)

ADD_CUSTOM_TARGET(java_basic ALL
  COMMAND ${JAVA_COMPILE} -d ${PROJECT_DIRECTORY}/examples/java/basic -cp ${CMAKE_CURRENT_BINARY_DIR}/simgrid.jar ${PROJECT_DIRECTORY}/examples/java/basic/*.java
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/simgrid.jar ${PROJECT_DIRECTORY}/examples/java/basic/*.java
)
	
ADD_CUSTOM_TARGET(java_ping_pong ALL
  COMMAND ${JAVA_COMPILE} -d ${PROJECT_DIRECTORY}/examples/java/ping_pong -cp ${CMAKE_CURRENT_BINARY_DIR}/simgrid.jar ${PROJECT_DIRECTORY}/examples/java/ping_pong/*.java
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/simgrid.jar ${PROJECT_DIRECTORY}/examples/java/ping_pong/*.java
)
	
ADD_CUSTOM_TARGET(java_comm_time ALL
  COMMAND ${JAVA_COMPILE} -d ${PROJECT_DIRECTORY}/examples/java/comm_time -cp ${CMAKE_CURRENT_BINARY_DIR}/simgrid.jar ${PROJECT_DIRECTORY}/examples/java/comm_time/*.java
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/simgrid.jar ${PROJECT_DIRECTORY}/examples/java/comm_time/*.java
)
	
ADD_CUSTOM_TARGET(java_suspend ALL
  COMMAND ${JAVA_COMPILE} -d ${PROJECT_DIRECTORY}/examples/java/suspend -cp ${CMAKE_CURRENT_BINARY_DIR}/simgrid.jar ${PROJECT_DIRECTORY}/examples/java/suspend/*.java
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/simgrid.jar ${PROJECT_DIRECTORY}/examples/java/suspend/*.java
)
