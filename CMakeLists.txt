cmake_minimum_required(VERSION 2.8)
project(SimGrid-Java C)
set(UPDATE_TYPE svn)
set(CMAKE_C_FLAGS "" CACHE TYPE INTERNAL FORCE)
set(CMAKE_EXE_LINKER_FLAGS "" CACHE TYPE INTERNAL FORCE)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_HOME_DIRECTORY}/lib")
set(soversion 1)

set(SIMGRID_JAVA_VERSION_MAJOR "1")
set(SIMGRID_JAVA_VERSION_MINOR "2")
set(SIMGRID_JAVA_VERSION_PATCH "0")
set(DIST_NAME ${PROJECT_NAME}-${SIMGRID_JAVA_VERSION_MAJOR}.${SIMGRID_JAVA_VERSION_MINOR})

########
## GCC #
########
exec_program("${CMAKE_C_COMPILER} --version" OUTPUT_VARIABLE "COMPILER_C_VERSION")
string(REGEX MATCH "[0-9].[0-9].[0-9]" COMPILER_C_VERSION "${COMPILER_C_VERSION}")
string(REGEX MATCH "^[0-9].[0-9]" COMPILER_C_VERSION_MAJOR_MINOR "${COMPILER_C_VERSION}")
string(REPLACE "${COMPILER_C_VERSION_MAJOR_MINOR}." "" COMPILER_C_VERSION_PATCH "${COMPILER_C_VERSION}")

########
## GIT #
########
if(EXISTS ${CMAKE_HOME_DIRECTORY}/.git/ AND NOT WIN32)
exec_program("git remote | head -n 1" OUTPUT_VARIABLE remote RETURN_VALUE ret)
exec_program("git config --get remote.${remote}.url" OUTPUT_VARIABLE url RETURN_VALUE ret)

if(url)
	exec_program("git --git-dir=${CMAKE_HOME_DIRECTORY}/.git log --oneline -1" OUTPUT_VARIABLE "GIT_VERSION")
	message(STATUS "Git version: ${GIT_VERSION}")
	exec_program("git --git-dir=${CMAKE_HOME_DIRECTORY}/.git log -n 1 --format=%ai ." OUTPUT_VARIABLE "GIT_DATE")
	message(STATUS "Git date: ${GIT_DATE}")
	string(REGEX REPLACE " .*" "" GIT_VERSION "${GIT_VERSION}")
	STRING(REPLACE " +0000" "" GIT_DATE ${GIT_DATE})
	STRING(REPLACE " " "~" GIT_DATE ${GIT_DATE})
	STRING(REPLACE ":" "-" GIT_DATE ${GIT_DATE})
endif(url)
endif(EXISTS ${CMAKE_HOME_DIRECTORY}/.git/ AND NOT WIN32)

#########
# CDASH #
#########
SET(DROP_METHOD "http")
SET(DROP_SITE "cdash.inria.fr/CDash")
SET(DROP_LOCATION "/submit.php?project=simgrid")
SET(DROP_SITE_CDASH TRUE)
SET(TRIGGER_SITE "http://cdash.inria.fr/CDash/cgi-bin/Submit-Random-TestingResults.cgi")
SET(BUILDNAME "SimGrid-Java" CACHE TYPE INTERNAL FORCE)
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
	src/jmsg_comm.c
	src/jmsg_comm.h
	src/jmsg_host.c
	src/jmsg_host.h
	src/jmsg_process.c
	src/jmsg_process.h
	src/jmsg_task.c
	src/jmsg_task.h
	src/jmsg_synchro.c
	src/jmsg_synchro.h
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
	org/simgrid/msg/NativeException.java
	org/simgrid/msg/Process.java
	org/simgrid/msg/ProcessKilledError.java
	org/simgrid/msg/ProcessNotFoundException.java
	org/simgrid/msg/Task.java
	org/simgrid/msg/TaskCancelledException.java
	org/simgrid/msg/TimeoutException.java
	org/simgrid/msg/TransferFailureException.java	
	org/simgrid/msg/Mutex.java
	org/simgrid/msg/Comm.java
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
	examples/master_slave_bypass/Master.java
	examples/master_slave_bypass/MsBypass.java
	examples/master_slave_bypass/Slave.java
	examples/master_slave_bypass/FinalizeTask.java
	examples/master_slave_kill/Master.java
	examples/master_slave_kill/MsKill.java
	examples/master_slave_kill/Slave.java
	examples/master_slave_kill/FinalizeTask.java
	examples/async/AsyncTest.java
	examples/async/FinalizeTask.java  
	examples/async/Forwarder.java
	examples/async/Master.java  
	examples/async/Slave.java
)

set(CMAKE_SRC
${CMAKE_HOME_DIRECTORY}/ChangeLog
${CMAKE_HOME_DIRECTORY}/CMakeLists.txt
${CMAKE_HOME_DIRECTORY}/FindJavaSG.cmake
${CMAKE_HOME_DIRECTORY}/FindSimGrid.cmake
)

set(XML_FILES
    examples/commTime/commTimeDeployment.xml
    examples/mutualExclusion/ring3.xml
    examples/mutualExclusion/centralized/mutex_centralized_deployment.xml
    examples/platform.xml
    examples/pingPong/pingPongDeployment.xml
    examples/basic/basicDeployment.xml
    examples/basic/deploy.xml
    examples/master_slave_bypass/platform.xml
    examples/master_slave_kill/platform.xml
    examples/async/asyncDeployment.xml
)

set(source_to_pack
${CMAKE_SRC}
${JMSG_C_SRC}
${JMSG_JAVA_SRC}
${JAVA_EXAMPLES}
${XML_FILES}
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
set(INCLUDE_PATH "-I${CMAKE_HOME_DIRECTORY}/src -I${SIMGRID_INCLUDES} ")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${INCLUDE_PATH}")

if(COMPILER_C_VERSION_MAJOR_MINOR MATCHES "4.6")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error=unused-but-set-variable")
endif(COMPILER_C_VERSION_MAJOR_MINOR MATCHES "4.6")

###########
# TARGETS #
###########
# SG_java
add_library(SG_java SHARED ${JMSG_C_SRC})
if(WIN32)
    set_target_properties(SG_java PROPERTIES LINK_FLAGS "-Wl,--subsystem,windows,--kill-at ${SIMGRID_LIB}" PREFIX "")
    find_path(PEXPORTS_PATH NAMES pexports.exe PATHS NO_DEFAULT_PATHS)
    message(STATUS "pexports: ${PEXPORTS_PATH}")
    if(PEXPORTS_PATH)
    add_custom_command(TARGET SG_java POST_BUILD
    COMMAND ${PEXPORTS_PATH}/pexports.exe ${CMAKE_BINARY_DIR}/SG_java.dll > ${CMAKE_BINARY_DIR}/SG_java.def)
    endif(PEXPORTS_PATH)
else(WIN32)
    target_link_libraries(SG_java pcre pthread simgrid)
    set_target_properties(SG_java PROPERTIES LINK_FLAGS "-L${SIMGRID_LIB_PATH} -L$ENV{SIMGRID_PCRE_LIBRARY_PATH}/lib")
endif(WIN32)

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
	DEPENDS ${JAVA_EXAMPLES}
  	COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/basic/*.java  
  	COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/async/*.java
 	COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/pingPong/*.java
  	COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/commTime/*.java
  	COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/mutualExclusion/centralized/*.java              			
	COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/master_slave_bypass/*.java
	COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/master_slave_kill/*.java
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

ADD_TEST(basic           ${TESH_BIN_PATH} ${TESH_OPTION} --setenv srcdir=${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/examples/basic/basic.tesh)
ADD_TEST(async           ${TESH_BIN_PATH} ${TESH_OPTION} --setenv srcdir=${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/examples/async/async.tesh)
ADD_TEST(pingPong        ${TESH_BIN_PATH} ${TESH_OPTION} --setenv srcdir=${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/examples/pingPong/pingpong.tesh)
ADD_TEST(CommTime        ${TESH_BIN_PATH} ${TESH_OPTION} --setenv srcdir=${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/examples/commTime/commtime.tesh)
ADD_TEST(mutualExclusion ${TESH_BIN_PATH} ${TESH_OPTION} --setenv srcdir=${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/examples/mutualExclusion/mutualexclusion.tesh)
ADD_TEST(bypass          ${TESH_BIN_PATH} ${TESH_OPTION} --setenv srcdir=${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/examples/master_slave_bypass/bypass.tesh)
ADD_TEST(kill            ${TESH_BIN_PATH} ${TESH_OPTION} --setenv srcdir=${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/examples/master_slave_kill/kill.tesh)
#Don't forget to put new test in this list!!!
set(test_list basic async pingPong CommTime mutualExclusion bypass kill)

##########################################
# Set the  DYLD_LIBRARY_PATH for mac     #
##########################################
if(APPLE)
    if($ENV{DYLD_LIBRARY_PATH} STREQUAL "")
        set_tests_properties(${test_list}
        PROPERTIES ENVIRONMENT "DYLD_LIBRARY_PATH=${CMAKE_HOME_DIRECTORY}/lib:${SIMGRID_LIB_PATH}")
    else($ENV{DYLD_LIBRARY_PATH} STREQUAL "")
        set_tests_properties(${test_list}
        PROPERTIES ENVIRONMENT "DYLD_LIBRARY_PATH=$ENV{DYLD_LIBRARY_PATH}:${CMAKE_HOME_DIRECTORY}/lib:${SIMGRID_LIB_PATH}")
    endif($ENV{DYLD_LIBRARY_PATH} STREQUAL "")
endif(APPLE)

##########################################
# Set the  LD_LIBRARY_PATH for our tests #
##########################################
if(NOT WIN32)
if($ENV{LD_LIBRARY_PATH} STREQUAL "")
    set_tests_properties(${test_list}
    PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_HOME_DIRECTORY}/lib:${SIMGRID_LIB_PATH}")
else($ENV{LD_LIBRARY_PATH} STREQUAL "")
    set_tests_properties(${test_list}
    PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=$ENV{LD_LIBRARY_PATH}:${CMAKE_HOME_DIRECTORY}/lib:${SIMGRID_LIB_PATH}")
endif($ENV{LD_LIBRARY_PATH} STREQUAL "")
endif(NOT WIN32)

###########
# Install #
###########
install(TARGETS SG_java
	DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/java/)
install(FILES ${CMAKE_HOME_DIRECTORY}/simgrid.jar
	DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/java/)


#########
# Dist  #
#########

add_custom_target(dist-dir
  COMMENT "Generating the distribution directory"
  COMMAND test -e ${DIST_NAME}/ && chmod -R a+w ${DIST_NAME}/ || true
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${DIST_NAME}/
  COMMAND ${CMAKE_COMMAND} -E make_directory ${DIST_NAME}
)

set(dirs_in_tarball "")
foreach(file ${source_to_pack})

  # This damn prefix is still set somewhere (seems to be in subdirs)
  string(REPLACE "${CMAKE_HOME_DIRECTORY}/" "" file "${file}")
  
  # Create the directory on need
  get_filename_component(file_location ${file} PATH)
  string(REGEX MATCH ";${file_location};" OPERATION "${dirs_in_tarball}")
  if(NOT OPERATION)
       set(dirs_in_tarball "${dirs_in_tarball};${file_location};")
       add_custom_command(
         TARGET dist-dir
         COMMAND ${CMAKE_COMMAND} -E make_directory ${DIST_NAME}/${file_location}/
       )       
   endif(NOT OPERATION)
   
   # Actually copy the file
   add_custom_command(
     TARGET dist-dir
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/${file} ${DIST_NAME}/${file_location}/
   )
endforeach(file ${source_to_pack})

add_custom_target(dist
  DEPENDS ${CMAKE_BINARY_DIR}/${DIST_NAME}.tar.gz
  COMMENT "Generated ${DIST_NAME}"
)
add_custom_command(
	OUTPUT ${CMAKE_BINARY_DIR}/${DIST_NAME}.tar.gz	
	COMMENT "Compressing the archive from the distribution directory"
	COMMAND ${CMAKE_COMMAND} -E tar cf ${DIST_NAME}.tar ${DIST_NAME}/
  	COMMAND gzip -9v ${DIST_NAME}.tar
  	COMMAND ${CMAKE_COMMAND} -E remove_directory ${DIST_NAME}/
)
add_dependencies(dist dist-dir)

########
# NOTE #
########
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/Testing/Notes/)
file(WRITE ${CMAKE_BINARY_DIR}/Testing/Notes/Build  "GIT version : ${GIT_VERSION}\n")
file(APPEND ${CMAKE_BINARY_DIR}/Testing/Notes/Build "Release     : ${DIST_NAME}\n")
file(APPEND ${CMAKE_BINARY_DIR}/Testing/Notes/Build "Pipol user  : $ENV{PIPOL_USER}\n")
file(APPEND ${CMAKE_BINARY_DIR}/Testing/Notes/Build "Pipol image : $ENV{PIPOL_IMAGE}\n")

#######
# DOC #
#######
set(WARNING "This file is generated, don't edit it!")
set(srcdir ${CMAKE_HOME_DIRECTORY})
include(GenerateDoc.cmake)
