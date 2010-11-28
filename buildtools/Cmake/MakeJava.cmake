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

ADD_CUSTOM_COMMAND(
	OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/examples/java/basic/BasicTest.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/java/basic/FinalizeTask.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/java/basic/Forwarder.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/java/basic/Slave.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/java/basic/Master.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/java/ping_pong/PingPongTest.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/java/ping_pong/Sender.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/java/ping_pong/PingPongTask.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/java/ping_pong/Receiver.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/java/comm_time/FinalizeTask.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/java/comm_time/CommTimeTest.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/java/comm_time/Slave.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/java/comm_time/Master.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/java/suspend/SuspendTest.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/java/suspend/LazyGuy.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/java/suspend/DreamMaster.class
			
	DEPENDS simgrid
            ${CMAKE_CURRENT_BINARY_DIR}/simgrid.jar
			${PROJECT_DIRECTORY}/examples/java/basic/*.java
			${PROJECT_DIRECTORY}/examples/java/ping_pong/*.java
			${PROJECT_DIRECTORY}/examples/java/comm_time/*.java
			${PROJECT_DIRECTORY}/examples/java/suspend/*.java
			
	COMMENT "Build examples for java"	
	
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/examples/java/basic
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/examples/java/ping_pong
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/examples/java/comm_time
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/examples/java/suspend
	
  	COMMAND ${JAVA_COMPILE} -d ${CMAKE_CURRENT_BINARY_DIR}/examples/java/basic -cp ${CMAKE_CURRENT_BINARY_DIR}/simgrid.jar ${PROJECT_DIRECTORY}/examples/java/basic/*.java  
 	COMMAND ${JAVA_COMPILE} -d ${CMAKE_CURRENT_BINARY_DIR}/examples/java/ping_pong -cp ${CMAKE_CURRENT_BINARY_DIR}/simgrid.jar ${PROJECT_DIRECTORY}/examples/java/ping_pong/*.java
  	COMMAND ${JAVA_COMPILE} -d ${CMAKE_CURRENT_BINARY_DIR}/examples/java/comm_time -cp ${CMAKE_CURRENT_BINARY_DIR}/simgrid.jar ${PROJECT_DIRECTORY}/examples/java/comm_time/*.java
  	COMMAND ${JAVA_COMPILE} -d ${CMAKE_CURRENT_BINARY_DIR}/examples/java/suspend -cp ${CMAKE_CURRENT_BINARY_DIR}/simgrid.jar ${PROJECT_DIRECTORY}/examples/java/suspend/*.java
)

ADD_CUSTOM_TARGET(simgrid_java_examples ALL
                  DEPENDS 	${CMAKE_CURRENT_BINARY_DIR}/examples/java/basic/BasicTest.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/java/basic/FinalizeTask.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/java/basic/Forwarder.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/java/basic/Slave.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/java/basic/Master.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/java/ping_pong/PingPongTest.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/java/ping_pong/Sender.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/java/ping_pong/PingPongTask.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/java/ping_pong/Receiver.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/java/comm_time/FinalizeTask.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/java/comm_time/CommTimeTest.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/java/comm_time/Slave.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/java/comm_time/Master.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/java/suspend/SuspendTest.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/java/suspend/LazyGuy.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/java/suspend/DreamMaster.class
)