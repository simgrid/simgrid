cmake_minimum_required(VERSION 2.8)
project(simgrid-java C)
set(UPDATE_TYPE svn)
set(CMAKE_C_FLAGS "" CACHE TYPE INTERNAL FORCE)
set(CMAKE_EXE_LINKER_FLAGS "" CACHE TYPE INTERNAL FORCE)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_HOME_DIRECTORY}/lib")
set(soversion 1)

########
## SVN #
########
if(IS_DIRECTORY ${CMAKE_HOME_DIRECTORY}/.svn)
	find_file(SVN ".svn" ${CMAKE_HOME_DIRECTORY})
	exec_program("svnversion ${CMAKE_HOME_DIRECTORY}" OUTPUT_VARIABLE "SVN_VERSION")
endif(IS_DIRECTORY ${CMAKE_HOME_DIRECTORY}/.svn)

exec_program("git config --get svn-remote.svn.url"
	OUTPUT_VARIABLE url
	RETURN_VALUE ret)
if(ret EQUAL 0)
	exec_program("git svn info" ${CMAKE_HOME_DIRECTORY}
		OUTPUT_VARIABLE "GIT_SVN_VERSION")
	string(REPLACE "\n" ";" GIT_SVN_VERSION ${GIT_SVN_VERSION})
	foreach(line ${GIT_SVN_VERSION})
		string(REGEX MATCH "^Revision:.*" line_good ${line})
		if(line_good)
			string(REPLACE "Revision: " ""
				line_good ${line_good})
			set(SVN_VERSION ${line_good})
		endif(line_good)
	endforeach(line ${GIT_SVN_VERSION})
endif(ret EQUAL 0)

message(STATUS "Svn version : ${SVN_VERSION}")

#########
# CDASH #
#########
SET(DROP_METHOD "http")
SET(DROP_SITE "cdash.inria.fr/CDash")
SET(DROP_LOCATION "/submit.php?project=simgrid")
SET(DROP_SITE_CDASH TRUE)
SET(TRIGGER_SITE "http://cdash.inria.fr/CDash/cgi-bin/Submit-Random-TestingResults.cgi")
SET(BUILDNAME "Simgrid-Java" CACHE TYPE INTERNAL FORCE)
set(osname ${CMAKE_SYSTEM_NAME})
set(cpu ${CMAKE_SYSTEM_PROCESSOR})
set(DISTRIB2 ${CMAKE_SYSTEM_VERSION})
SET(SITE "${osname}_${DISTRIB2}_${cpu}")
SET(CTEST_SITE "${osname}_${DISTRIB2}_${cpu}")
SET(CTEST_PROJECT_NAME "${PROJECT_NAME}")
SET(CTEST_CUSTOM_MAXIMUM_FAILED_TEST_OUTPUT_SIZE "3000000")
SET(CTEST_CUSTOM_MAXIMUM_PASSED_TEST_OUTPUT_SIZE "3000000")
set(PIPOL_IMAGE $ENV{PIPOL_IMAGE})
if(NOT ${PIPOL_IMAGE} MATCHES "\n")
set(SITE ${PIPOL_IMAGE})
endif(NOT ${PIPOL_IMAGE} MATCHES "\n")
message(STATUS "SITE        : ${SITE}")
message(STATUS "BUILDNAME   : ${BUILDNAME}")

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

set(JAVA_EXAMPLES
	examples/mutualExclusion/centralized/MutexCentral.java
	examples/mutualExclusion/centralized/GrantTask.java
	examples/mutualExclusion/centralized/Coordinator.java
	examples/mutualExclusion/centralized/ReleaseTask.java
	examples/mutualExclusion/centralized/Node.java
	examples/mutualExclusion/centralized/RequestTask.java
	examples/basic/FinalizeTask.java
	examples/basic/Forwarder.java
	examples/basic/Master.java
	examples/basic/BasicTest.java
	examples/basic/Slave.java
	examples/commTime/FinalizeTask.java
	examples/commTime/Master.java
	examples/commTime/Slave.java
	examples/commTime/CommTimeTest.java
	examples/pingPong/PingPongTask.java
	examples/pingPong/Sender.java
	examples/pingPong/Receiver.java
	examples/pingPong/PingPongTest.java
)

string(REPLACE "org/" "${CMAKE_HOME_DIRECTORY}/org/" JAVA_FILES "${JMSG_JAVA_SRC}") 
string(REPLACE "org/" "${CMAKE_HOME_DIRECTORY}/classes/org/" JMSG_JAVA_SRC_CLASS "${JMSG_JAVA_SRC}")
string(REPLACE ".java" ".class" JMSG_JAVA_SRC_CLASS "${JMSG_JAVA_SRC_CLASS}")
string(REPLACE "examples/" "${CMAKE_HOME_DIRECTORY}/examples/" JAVA_EXAMPLES_CLASS "${JAVA_EXAMPLES}") 
string(REPLACE ".java" ".class" JAVA_EXAMPLES_CLASS "${JAVA_EXAMPLES_CLASS}")

#########
# FLAGS #
#########
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g -Wall -Wunused -Wmissing-prototypes -Wmissing-declarations -Wpointer-arith -Wchar-subscripts -Wcomment -Wformat -Wwrite-strings -Wno-unused-function -Wno-unused-parameter -Wno-strict-aliasing -Wno-format-nonliteral -Werror ")
set(INCLUDE_PATH "-I${CMAKE_HOME_DIRECTORY}/src -I$ENV{SIMGRID_ROOT}/include -I$ENV{SIMGRID_ROOT}/src -I$ENV{SIMGRID_ROOT}/src/include")
set(LIB_PATH "-L$ENV{SIMGRID_ROOT}/lib")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${INCLUDE_PATH} ${LIB_PATH}")

###########
# TARGETS #
###########
# SG_java
add_library(SG_java SHARED ${JMSG_C_SRC})
target_link_libraries(SG_java -lsimgrid)
set_target_properties(SG_java PROPERTIES VERSION ${soversion})
  
# java_classes
add_custom_command(
	OUTPUT 	${JMSG_JAVA_SRC_CLASS}
	DEPENDS ${JMSG_JAVA_SRC}
	COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_HOME_DIRECTORY}/classes/"
	COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/classes/
  					-cp ${CMAKE_HOME_DIRECTORY}/classes/ ${JAVA_FILES}
	COMMENT "Compiling java sources of core library..."					
)
add_custom_target(java_classes ALL
  DEPENDS ${JMSG_JAVA_SRC_CLASS}		
)

# simgrid_jar
add_custom_target(simgrid_jar ALL
  COMMAND ${JAVA_ARCHIVE} -cvf ${CMAKE_HOME_DIRECTORY}/simgrid.jar .
  COMMENT "Building simgrid.jar..."
  WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/classes 
)

# simgrid_java_examples
add_custom_command(
	COMMENT "Build examples for java"
	OUTPUT	${JAVA_EXAMPLES_CLASS}
	
  	COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/basic/*.java  
 	COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/pingPong/*.java
  	COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/commTime/*.java
  	COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/mutualExclusion/centralized/*.java              			
)
add_custom_target(simgrid_java_examples ALL
	DEPENDS ${JAVA_EXAMPLES_CLASS}
)

########################
# Declare dependencies #
########################
add_dependencies(simgrid_jar java_classes)
add_dependencies(simgrid_java_examples simgrid_jar SG_java)

SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
${CMAKE_HOME_DIRECTORY}/simgrid.jar
)

#####################
# Declare our tests #
#####################
INCLUDE(CTest)
ENABLE_TESTING()

ADD_TEST(basic
java -cp .:${CMAKE_HOME_DIRECTORY}/examples:${CMAKE_HOME_DIRECTORY}/simgrid.jar basic/BasicTest ${CMAKE_HOME_DIRECTORY}/examples/platform.xml ${CMAKE_HOME_DIRECTORY}/examples/basic/basicDeployment.xml
)
ADD_TEST(pingPong
java -cp .:${CMAKE_HOME_DIRECTORY}/examples:${CMAKE_HOME_DIRECTORY}/simgrid.jar pingPong/PingPongTest ${CMAKE_HOME_DIRECTORY}/examples/platform.xml ${CMAKE_HOME_DIRECTORY}/examples/pingPong/pingPongDeployment.xml
)
ADD_TEST(CommTime
java -cp .:${CMAKE_HOME_DIRECTORY}/examples:${CMAKE_HOME_DIRECTORY}/simgrid.jar commTime/CommTimeTest ${CMAKE_HOME_DIRECTORY}/examples/platform.xml ${CMAKE_HOME_DIRECTORY}/examples/commTime/commTimeDeployment.xml
)
ADD_TEST(mutualExclusion
java -cp .:${CMAKE_HOME_DIRECTORY}/examples:${CMAKE_HOME_DIRECTORY}/simgrid.jar mutualExclusion/centralized/MutexCentral ${CMAKE_HOME_DIRECTORY}/examples/mutualExclusion/ring3.xml ${CMAKE_HOME_DIRECTORY}/examples/mutualExclusion/centralized/mutex_centralized_deployment.xml
)

##########################################
# Set the  LD_LIBRARY_PATH for our tests #
##########################################
if($ENV{LD_LIBRARY_PATH} STREQUAL "")
    set_tests_properties(basic pingPong CommTime mutualExclusion
    PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_HOME_DIRECTORY}/lib:$ENV{SIMGRID_ROOT}/lib")
else($ENV{LD_LIBRARY_PATH} STREQUAL "")
    set_tests_properties(basic pingPong CommTime mutualExclusion
    PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=$ENV{LD_LIBRARY_PATH}:${CMAKE_HOME_DIRECTORY}/lib:$ENV{SIMGRID_ROOT}/lib")
endif($ENV{LD_LIBRARY_PATH} STREQUAL "")

###########
# Install #
###########
install(TARGETS SG_java
	DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/java/)
install(FILES ${CMAKE_HOME_DIRECTORY}/simgrid.jar
	DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/java/)

########
# NOTE #
########
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/Testing/Notes/)
file(WRITE ${CMAKE_BINARY_DIR}/Testing/Notes/Build  "SVN version : ${SVN_VERSION}\n")
file(APPEND ${CMAKE_BINARY_DIR}/Testing/Notes/Build "Release     : simgrid-java-${soversion}\n")
file(APPEND ${CMAKE_BINARY_DIR}/Testing/Notes/Build "Pipol user  : $ENV{PIPOL_USER}\n")
file(APPEND ${CMAKE_BINARY_DIR}/Testing/Notes/Build "Pipol image : $ENV{PIPOL_IMAGE}\n")