cmake_minimum_required(VERSION 2.6)
project(simgrid-java C)
include(FindSimGrid.cmake)

set(soversion 1)
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

file(GLOB_RECURSE examples_sources
	"examples/*.java"
	"examples/*.xml"
	"examples/*.txt"
	"examples/java/runtest"
)

###############
# Build Stuff #
###############
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${HAVE_GRAS_H} -Wall -Wunused -Wmissing-prototypes -Wmissing-declarations -Wpointer-arith -Wchar-subscripts -Wcomment -Wformat -Wwrite-strings -Wno-unused-function -Wno-unused-parameter -Wno-strict-aliasing -Wno-format-nonliteral -Werror ")

add_library(simgrid-java SHARED ${JMSG_C_SRC})
set_target_properties(simgrid-java PROPERTIES VERSION ${soversion})
target_link_libraries(simgrid-java ${HAVE_SIMGRID_LIB})

set(JAVA_FILES ${JMSG_JAVA_SRC})
set(JAVA_CLASSES ${JAVA_FILES})

string(REPLACE "org/" "${CMAKE_HOME_DIRECTORY}/org/"
               JAVA_FILES "${JAVA_FILES}")

string(REPLACE "org/simgrid/msg" "${CMAKE_BINARY_DIR}/classes/simgrid/msg"
               JAVA_CLASSES "${JAVA_CLASSES}")
string(REPLACE ".java" ".class;" 
               JAVA_CLASSES "${JAVA_CLASSES}")
	       

add_custom_command(
  OUTPUT  ${CMAKE_BINARY_DIR}/classes/
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/classes/")


# compile all .java files with javac to .class
ADD_CUSTOM_COMMAND(
  OUTPUT ${JAVA_CLASSES}
  DEPENDS ${JAVA_FILES} ${CMAKE_BINARY_DIR}/classes/
  COMMAND ${JAVA_COMPILE} -d ${CMAKE_BINARY_DIR}/classes/
                          -cp ${CMAKE_BINARY_DIR}/classes/
			  ${JAVA_FILES}
  COMMENT "Compiling java sources of core library..."
)

ADD_CUSTOM_TARGET(simgrid_jar ALL
                  DEPENDS ${CMAKE_BINARY_DIR}/share/simgrid.jar)

ADD_CUSTOM_COMMAND( 
  OUTPUT ${CMAKE_BINARY_DIR}/share/simgrid.jar
  DEPENDS ${JAVA_CLASSES} 
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/classes
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/share
  COMMAND ${JAVA_ARCHIVE} -cvf ${CMAKE_BINARY_DIR}/share/simgrid.jar .
  COMMENT "Building simgrid.jar..."
)

ADD_CUSTOM_COMMAND(
	OUTPUT  ${CMAKE_BINARY_DIR}/examples/basic/BasicTest.class
			${CMAKE_BINARY_DIR}/examples/basic/FinalizeTask.class
			${CMAKE_BINARY_DIR}/examples/basic/Forwarder.class
			${CMAKE_BINARY_DIR}/examples/basic/Slave.class
			${CMAKE_BINARY_DIR}/examples/basic/Master.class
			${CMAKE_BINARY_DIR}/examples/pingPong/PingPongTest.class
			${CMAKE_BINARY_DIR}/examples/pingPong/Sender.class
			${CMAKE_BINARY_DIR}/examples/pingPong/PingPongTask.class
			${CMAKE_BINARY_DIR}/examples/pingPong/Receiver.class
			${CMAKE_BINARY_DIR}/examples/commTime/FinalizeTask.class
			${CMAKE_BINARY_DIR}/examples/commTime/CommTimeTest.class
			${CMAKE_BINARY_DIR}/examples/commTime/Slave.class
			${CMAKE_BINARY_DIR}/examples/commTime/Master.class
			${CMAKE_BINARY_DIR}/examples/suspend/SuspendTest.class
			${CMAKE_BINARY_DIR}/examples/suspend/LazyGuy.class
			${CMAKE_BINARY_DIR}/examples/suspend/DreamMaster.class
			
	DEPENDS 
            ${CMAKE_BINARY_DIR}/share/simgrid.jar
			${CMAKE_HOME_DIRECTORY}/examples/basic/*.java
			${CMAKE_HOME_DIRECTORY}/examples/pingPong/*.java
			${CMAKE_HOME_DIRECTORY}/examples/commTime/*.java
			${CMAKE_HOME_DIRECTORY}/examples/suspend/*.java
			
	COMMENT "Build examples for java"	
	
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/examples/basic
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/examples/ping_pong
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/examples/comm_time
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/examples/suspend
	
  	COMMAND ${JAVA_COMPILE} -d ${CMAKE_BINARY_DIR}/examples/basic -cp ${CMAKE_BINARY_DIR}/share/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/basic/*.java  
 	COMMAND ${JAVA_COMPILE} -d ${CMAKE_BINARY_DIR}/examples/ping_pong -cp ${CMAKE_BINARY_DIR}/share/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/pingPong/*.java
  	COMMAND ${JAVA_COMPILE} -d ${CMAKE_BINARY_DIR}/examples/comm_time -cp ${CMAKE_BINARY_DIR}/share/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/commTime/*.java
  	COMMAND ${JAVA_COMPILE} -d ${CMAKE_BINARY_DIR}/examples/suspend -cp ${CMAKE_BINARY_DIR}/share/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/suspend/*.java
)

ADD_CUSTOM_TARGET(examples ALL
	DEPENDS ${CMAKE_BINARY_DIR}/examples/basic/BasicTest.class
		${CMAKE_BINARY_DIR}/examples/basic/FinalizeTask.class
		${CMAKE_BINARY_DIR}/examples/basic/Forwarder.class
		${CMAKE_BINARY_DIR}/examples/basic/Slave.class
		${CMAKE_BINARY_DIR}/examples/basic/Master.class
		${CMAKE_BINARY_DIR}/examples/ping_pong/PingPongTest.class
		${CMAKE_BINARY_DIR}/examples/ping_pong/Sender.class
		${CMAKE_BINARY_DIR}/examples/ping_pong/PingPongTask.class
		${CMAKE_BINARY_DIR}/examples/ping_pong/Receiver.class
		${CMAKE_BINARY_DIR}/examples/comm_time/FinalizeTask.class
		${CMAKE_BINARY_DIR}/examples/comm_time/CommTimeTest.class
		${CMAKE_BINARY_DIR}/examples/comm_time/Slave.class
		${CMAKE_BINARY_DIR}/examples/comm_time/Master.class
		${CMAKE_BINARY_DIR}/examples/suspend/SuspendTest.class
		${CMAKE_BINARY_DIR}/examples/suspend/LazyGuy.class
		${CMAKE_BINARY_DIR}/examples/suspend/DreamMaster.class
	COMMENT Compiling the Java examples							
)

#####################
# Declare our tests #
#####################
ENABLE_TESTING()

set(osname ${CMAKE_SYSTEM_NAME})
set(cpu ${CMAKE_SYSTEM_PROCESSOR})
set(DISTRIB2 ${CMAKE_SYSTEM_VERSION})

SET(SITE "${osname}_${DISTRIB2}_${cpu}")
SET(CTEST_SITE "${osname}_${DISTRIB2}_${cpu}")
SET(CTEST_PROJECT_NAME "${PROJECT_NAME}")
SET(CTEST_CUSTOM_MAXIMUM_FAILED_TEST_OUTPUT_SIZE "3000000")
SET(CTEST_CUSTOM_MAXIMUM_PASSED_TEST_OUTPUT_SIZE "3000000")


# java examples
ADD_TEST(java-basic 	java -cp ${CMAKE_BINARY_DIR}/examples/basic:${CMAKE_BINARY_DIR}/share/simgrid.jar BasicTest examples/platform.xml ${CMAKE_HOME_DIRECTORY}/examples/basic/basic_deployment.xml)
ADD_TEST(java-pingpong 	java -cp ${CMAKE_BINARY_DIR}/examples/pingpong:${CMAKE_BINARY_DIR}/share/simgrid.jar PingpongTest examples/platform.xml ${CMAKE_HOME_DIRECTORY}/examples/pingpong/pingpong_deployment.xml)
ADD_TEST(java-pingpong 	java -cp ${CMAKE_BINARY_DIR}/examples/pingpong:${CMAKE_BINARY_DIR}/share/simgrid.jar PingpongTest examples/platform.xml ${CMAKE_HOME_DIRECTORY}/examples/pingpong/pingpong_deployment.xml)
#ADD_TEST(java-comm_time ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/test_java.sh ${CMAKE_HOME_DIRECTORY}/examples/java/comm_time CommTimeTest ${simgrid_BINARY_DIR})
#ADD_TEST(java-suspend 	${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/test_java.sh ${CMAKE_HOME_DIRECTORY}/examples/java/suspend SuspendTest ${simgrid_BINARY_DIR})

######################
# Install some stuff #
######################
install(FILES ${CMAKE_BINARY_DIR}/share/simgrid.jar
          DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/)

add_custom_target(uninstall
	COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/share/simgrid.jar
	COMMAND ${CMAKE_COMMAND} -E echo "uninstall binding java ok"
)	

install(TARGETS simgrid-java
        DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/)
