cmake_minimum_required(VERSION 2.6)
project(simgrid-java C)
include(FindSimGrid.cmake)

###############################
# Test the build dependencies #
###############################
SET(HAVE_JAVA 0)
include(FindJava)
include(FindJNI)
if(JAVA_INCLUDE_PATH)
	set(HAVE_JNI_H 1)
endif(JAVA_INCLUDE_PATH)	
if(JAVA_COMPILE AND JAVA_INCLUDE_PATH AND JAVA_INCLUDE_PATH2)
	SET(HAVE_JAVA 1)
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-I${JAVA_INCLUDE_PATH} ")
	if(NOT JAVA_INCLUDE_PATH STREQUAL JAVA_INCLUDE_PATH2)
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-I${JAVA_INCLUDE_PATH2} ")			
	endif(NOT JAVA_INCLUDE_PATH STREQUAL JAVA_INCLUDE_PATH2)
else(JAVA_COMPILE AND JAVA_INCLUDE_PATH AND JAVA_INCLUDE_PATH2) 
	SET(HAVE_JAVA 0)
endif(JAVA_COMPILE AND JAVA_INCLUDE_PATH AND JAVA_INCLUDE_PATH2)


###############################
# Declare our package content #
###############################

set(JMSG_C_SRC
	src/simix/smx_context_java.h
	src/simix/smx_context_java.c
	src/java/jxbt_utilities.c
	src/java/jxbt_utilities.h
	src/java/jmsg.c 
	src/java/jmsg.h
	src/java/jmsg_host.c
	src/java/jmsg_host.h
	src/java/jmsg_process.c
	src/java/jmsg_process.h
	src/java/jmsg_task.c
	src/java/jmsg_task.h
	src/java/jmsg_application_handler.c
	src/java/jmsg_application_handler.h
)

set(JMSG_JAVA_SRC
	org/simgrid/msg/ApplicationHandler.java
	org/simgrid/msg/Host.java
	org/simgrid/msg/HostFailureException.java	
	org/simgrid/msg/HostNotFoundException.java	
	org/simgrid/msg/JniException.java
	org/simgrid/msg/Msg.java
	org/simgrid/msg/MsgException.java
	org/simgrid/msg/MsgNative.java
	org/simgrid/msg/NativeException.java
	org/simgrid/msg/Process.java
	org/simgrid/msg/ProcessNotFoundException.java
	org/simgrid/msg/Sem.java
	org/simgrid/msg/Task.java
	org/simgrid/msg/TaskCancelledException.java
	org/simgrid/msg/TimeoutException.java
	org/simgrid/msg/TransferFailureException.java	
)

file(GLOB_RECURSE examples_sources
	"examples/*.java"
	"examples/*.xml"
	"examples/*.txt"
	"examples/java/runtest"
)

###############
# Build Stuff #
###############

set(JAVA_FILES ${JMSG_JAVA_SRC})
set(JAVA_CLASSES ${JAVA_FILES})

string(REPLACE "org/" "${CMAKE_HOME_DIRECTORY}/org/"
               JAVA_FILES "${JAVA_FILES}")

string(REPLACE "org/simgrid/msg" "${CMAKE_CURRENT_BINARY_DIR}/classes/simgrid/msg"
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
                  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/simgrid.jar)

ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/simgrid.jar
  DEPENDS ${JAVA_CLASSES}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/classes  
  COMMAND ${JAVA_ARCHIVE} -cvf ${CMAKE_CURRENT_BINARY_DIR}/simgrid.jar .
  COMMENT "Building simgrid.jar..."
)

ADD_CUSTOM_COMMAND(
	OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/examples/basic/BasicTest.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/basic/FinalizeTask.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/basic/Forwarder.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/basic/Slave.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/basic/Master.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/ping_pong/PingPongTest.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/ping_pong/Sender.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/ping_pong/PingPongTask.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/ping_pong/Receiver.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/comm_time/FinalizeTask.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/comm_time/CommTimeTest.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/comm_time/Slave.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/comm_time/Master.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/suspend/SuspendTest.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/suspend/LazyGuy.class
			${CMAKE_CURRENT_BINARY_DIR}/examples/suspend/DreamMaster.class
			
	DEPENDS 
            ${CMAKE_CURRENT_BINARY_DIR}/simgrid.jar
			${CMAKE_HOME_DIRECTORY}/examples/basic/*.java
			${CMAKE_HOME_DIRECTORY}/examples/ping_pong/*.java
			${CMAKE_HOME_DIRECTORY}/examples/comm_time/*.java
			${CMAKE_HOME_DIRECTORY}/examples/suspend/*.java
			
	COMMENT "Build examples for java"	
	
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/examples/basic
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/examples/ping_pong
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/examples/comm_time
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/examples/suspend
	
  	COMMAND ${JAVA_COMPILE} -d ${CMAKE_CURRENT_BINARY_DIR}/examples/basic -cp ${CMAKE_CURRENT_BINARY_DIR}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/basic/*.java  
 	COMMAND ${JAVA_COMPILE} -d ${CMAKE_CURRENT_BINARY_DIR}/examples/ping_pong -cp ${CMAKE_CURRENT_BINARY_DIR}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/ping_pong/*.java
  	COMMAND ${JAVA_COMPILE} -d ${CMAKE_CURRENT_BINARY_DIR}/examples/comm_time -cp ${CMAKE_CURRENT_BINARY_DIR}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/comm_time/*.java
  	COMMAND ${JAVA_COMPILE} -d ${CMAKE_CURRENT_BINARY_DIR}/examples/suspend -cp ${CMAKE_CURRENT_BINARY_DIR}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/suspend/*.java
)

ADD_CUSTOM_TARGET(simgrid_java_examples ALL
                  DEPENDS 	${CMAKE_CURRENT_BINARY_DIR}/examples/basic/BasicTest.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/basic/FinalizeTask.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/basic/Forwarder.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/basic/Slave.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/basic/Master.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/ping_pong/PingPongTest.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/ping_pong/Sender.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/ping_pong/PingPongTask.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/ping_pong/Receiver.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/comm_time/FinalizeTask.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/comm_time/CommTimeTest.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/comm_time/Slave.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/comm_time/Master.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/suspend/SuspendTest.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/suspend/LazyGuy.class
							${CMAKE_CURRENT_BINARY_DIR}/examples/suspend/DreamMaster.class
)