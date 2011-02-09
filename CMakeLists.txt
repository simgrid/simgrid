cmake_minimum_required(VERSION 2.6)
project(simgrid-java C)
set(CMAKE_C_FLAGS "" CACHE TYPE INTERNAL FORCE)
set(CMAKE_EXE_LINKER_FLAGS "" CACHE TYPE INTERNAL FORCE)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_HOME_DIRECTORY}/lib")


set(soversion 1)
###############################
# Test the build dependencies #
###############################
include(FindSimGrid.cmake)
include(FindJavaSG.cmake)

###############################
# Declare our package content #
###############################
set(JMSG_C_SRC
	src/smx_context_java.h
	src/smx_context_java.c
	src/jxbt_utilities.c
	src/jxbt_utilities.h
	src/jmsg.c 
	src/jmsg.h
	src/jmsg_host.c
	src/jmsg_host.h
	src/jmsg_process.c
	src/jmsg_process.h
	src/jmsg_task.c
	src/jmsg_task.h
	src/jmsg_application_handler.c
	src/jmsg_application_handler.h
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

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wunused -Wmissing-prototypes -Wmissing-declarations -Wpointer-arith -Wchar-subscripts -Wcomment -Wformat -Wwrite-strings -Wno-unused-function -Wno-unused-parameter -Wno-strict-aliasing -Wno-format-nonliteral -Werror ")

set(INCLUDE_PATH "-I${CMAKE_HOME_DIRECTORY}/src -I$ENV{SIMGRID_ROOT}/include -I$ENV{SIMGRID_ROOT}/src -I$ENV{SIMGRID_ROOT}/src/include")
set(LIB_PATH "-L$ENV{SIMGRID_ROOT}/lib")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${INCLUDE_PATH} ${LIB_PATH}")

add_library(SG_java SHARED ${JMSG_C_SRC})
target_link_libraries(SG_java -lsimgrid)
set_target_properties(SG_java PROPERTIES VERSION ${soversion})

set(JAVA_FILES ${JMSG_JAVA_SRC})
set(JAVA_CLASSES ${JAVA_FILES})

string(REPLACE "org/" "${CMAKE_HOME_DIRECTORY}/org/"
               JAVA_FILES "${JAVA_FILES}")

string(REPLACE "org/simgrid/msg" "${CMAKE_BINARY_DIR}/classes/simgrid/msg"
               JAVA_CLASSES "${JAVA_CLASSES}")
string(REPLACE ".java" ".class;" 
               JAVA_CLASSES "${JAVA_CLASSES}")	       

add_custom_command(
  OUTPUT  ${CMAKE_HOME_DIRECTORY}/classes/
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_HOME_DIRECTORY}/classes/"
  )
  
# compile all .java files with javac to .class
ADD_CUSTOM_COMMAND(
  OUTPUT ${JAVA_CLASSES}
  DEPENDS ${JMSG_JAVA_SRC} ${CMAKE_HOME_DIRECTORY}/classes/
  COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/classes/
                          -cp ${CMAKE_HOME_DIRECTORY}/classes/
			  ${JAVA_FILES}
  COMMENT "Compiling java sources of core library..."
)

ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_HOME_DIRECTORY}/simgrid.jar
  DEPENDS ${JAVA_CLASSES}
  WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/classes  
  COMMAND ${JAVA_ARCHIVE} -cvf ${CMAKE_HOME_DIRECTORY}/simgrid.jar .
  COMMENT "Building simgrid.jar..."
)

ADD_CUSTOM_COMMAND(
	OUTPUT  ${CMAKE_HOME_DIRECTORY}/examples/basic/BasicTest.class
			${CMAKE_HOME_DIRECTORY}/examples/basic/FinalizeTask.class
			${CMAKE_HOME_DIRECTORY}/examples/basic/Forwarder.class
			${CMAKE_HOME_DIRECTORY}/examples/basic/Slave.class
			${CMAKE_HOME_DIRECTORY}/examples/basic/Master.class
			${CMAKE_HOME_DIRECTORY}/examples/pingPong/PingPongTest.class
			${CMAKE_HOME_DIRECTORY}/examples/pingPong/Sender.class
			${CMAKE_HOME_DIRECTORY}/examples/pingPong/PingPongTask.class
			${CMAKE_HOME_DIRECTORY}/examples/pingPong/Receiver.class
			${CMAKE_HOME_DIRECTORY}/examples/pingPong/FinalizeTask.class
			${CMAKE_HOME_DIRECTORY}/examples/pingPong/CommTimeTest.class
			${CMAKE_HOME_DIRECTORY}/examples/pingPong/Slave.class
			${CMAKE_HOME_DIRECTORY}/examples/pingPong/Master.class
			${CMAKE_HOME_DIRECTORY}/lib/libsimgrid.so
			
	DEPENDS SG_java
            ${CMAKE_HOME_DIRECTORY}/simgrid.jar
			
	COMMENT "Build examples for java"	
	
  	COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/basic/*.java  
 	COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/pingPong/*.java
  	COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/pingPong/*.java
)

ADD_CUSTOM_TARGET(simgrid_java_examples ALL
                  DEPENDS 	${CMAKE_HOME_DIRECTORY}/examples/basic/BasicTest.class
							${CMAKE_HOME_DIRECTORY}/examples/basic/FinalizeTask.class
							${CMAKE_HOME_DIRECTORY}/examples/basic/Forwarder.class
							${CMAKE_HOME_DIRECTORY}/examples/basic/Slave.class
							${CMAKE_HOME_DIRECTORY}/examples/basic/Master.class
							${CMAKE_HOME_DIRECTORY}/examples/pingPong/PingPongTest.class
							${CMAKE_HOME_DIRECTORY}/examples/pingPong/Sender.class
							${CMAKE_HOME_DIRECTORY}/examples/pingPong/PingPongTask.class
							${CMAKE_HOME_DIRECTORY}/examples/pingPong/Receiver.class
							${CMAKE_HOME_DIRECTORY}/examples/pingPong/FinalizeTask.class
							${CMAKE_HOME_DIRECTORY}/examples/pingPong/CommTimeTest.class
							${CMAKE_HOME_DIRECTORY}/examples/pingPong/Slave.class
							${CMAKE_HOME_DIRECTORY}/examples/pingPong/Master.class
                  			
)

#####################
# Declare our tests #
#####################
INCLUDE(CTest)
ENABLE_TESTING()

ADD_TEST(java-basic 	${CMAKE_HOME_DIRECTORY}/test_java.sh ${CMAKE_HOME_DIRECTORY}/examples basic/BasicTest ${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/examples/basic/)
ADD_TEST(java-pingpong 	${CMAKE_HOME_DIRECTORY}/test_java.sh ${CMAKE_HOME_DIRECTORY}/examples pingPong/PingPongTest ${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/examples/pingPong/)