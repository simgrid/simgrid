cmake_minimum_required(VERSION 2.8)
project(SimGrid-Java C)
set(UPDATE_TYPE svn)
set(CMAKE_C_FLAGS "" CACHE TYPE INTERNAL FORCE)
set(CMAKE_EXE_LINKER_FLAGS "" CACHE TYPE INTERNAL FORCE)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_HOME_DIRECTORY}/lib")
set(soversion 1)

set(SIMGRID_JAVA_VERSION_MAJOR "3")
set(SIMGRID_JAVA_VERSION_MINOR "7")
set(SIMGRID_JAVA_VERSION_PATCH "1")
set(DIST_NAME ${PROJECT_NAME}-${SIMGRID_JAVA_VERSION_MAJOR}.${SIMGRID_JAVA_VERSION_MINOR}.${SIMGRID_JAVA_VERSION_PATCH})

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
  ${CMAKE_HOME_DIRECTORY}/src/smx_context_cojava.h
  ${CMAKE_HOME_DIRECTORY}/src/smx_context_cojava.c
  ${CMAKE_HOME_DIRECTORY}/src/jmsg_file.c
  ${CMAKE_HOME_DIRECTORY}/src/jmsg_file.h
  ${CMAKE_HOME_DIRECTORY}/src/smx_context_java.h
  ${CMAKE_HOME_DIRECTORY}/src/smx_context_java.c
  ${CMAKE_HOME_DIRECTORY}/src/jxbt_utilities.c
  ${CMAKE_HOME_DIRECTORY}/src/jxbt_utilities.h
  ${CMAKE_HOME_DIRECTORY}/src/jmsg.c 
  ${CMAKE_HOME_DIRECTORY}/src/jmsg.h
  ${CMAKE_HOME_DIRECTORY}/src/jmsg_comm.c
  ${CMAKE_HOME_DIRECTORY}/src/jmsg_comm.h
  ${CMAKE_HOME_DIRECTORY}/src/jmsg_host.c
  ${CMAKE_HOME_DIRECTORY}/src/jmsg_host.h
  ${CMAKE_HOME_DIRECTORY}/src/jmsg_process.c
  ${CMAKE_HOME_DIRECTORY}/src/jmsg_process.h
  ${CMAKE_HOME_DIRECTORY}/src/jmsg_task.c
  ${CMAKE_HOME_DIRECTORY}/src/jmsg_task.h
  ${CMAKE_HOME_DIRECTORY}/src/jmsg_synchro.c
  ${CMAKE_HOME_DIRECTORY}/src/jmsg_synchro.h
  ${CMAKE_HOME_DIRECTORY}/src/jmsg_rngstream.c
  ${CMAKE_HOME_DIRECTORY}/src/jmsg_rngstream.h
)

set(JTRACE_C_SRC
	src/jtrace.c
	src/jtrace.h
)	

set(JMSG_JAVA_SRC
  ${CMAKE_HOME_DIRECTORY}/org/simgrid/msg/Host.java
  ${CMAKE_HOME_DIRECTORY}/org/simgrid/msg/HostFailureException.java	
  ${CMAKE_HOME_DIRECTORY}/org/simgrid/msg/HostNotFoundException.java
  ${CMAKE_HOME_DIRECTORY}/org/simgrid/msg/File.java
  ${CMAKE_HOME_DIRECTORY}/org/simgrid/msg/JniException.java
  ${CMAKE_HOME_DIRECTORY}/org/simgrid/msg/Msg.java
  ${CMAKE_HOME_DIRECTORY}/org/simgrid/msg/MsgException.java
  ${CMAKE_HOME_DIRECTORY}/org/simgrid/msg/NativeException.java
  ${CMAKE_HOME_DIRECTORY}/org/simgrid/msg/Process.java
  ${CMAKE_HOME_DIRECTORY}/org/simgrid/msg/ProcessKilledError.java
  ${CMAKE_HOME_DIRECTORY}/org/simgrid/msg/ProcessNotFoundException.java
  ${CMAKE_HOME_DIRECTORY}/org/simgrid/msg/Task.java
  ${CMAKE_HOME_DIRECTORY}/org/simgrid/msg/TaskCancelledException.java
  ${CMAKE_HOME_DIRECTORY}/org/simgrid/msg/TimeoutException.java
  ${CMAKE_HOME_DIRECTORY}/org/simgrid/msg/TransferFailureException.java	
  ${CMAKE_HOME_DIRECTORY}/org/simgrid/msg/Mutex.java
  ${CMAKE_HOME_DIRECTORY}/org/simgrid/msg/Comm.java
  ${CMAKE_HOME_DIRECTORY}/org/simgrid/msg/RngStream.java
)
set(JTRACE_JAVA_SRC
	org/simgrid/trace/Trace.java
)

set(JAVA_EXAMPLES
  ${CMAKE_HOME_DIRECTORY}/examples/async/AsyncTest.java
  ${CMAKE_HOME_DIRECTORY}/examples/async/FinalizeTask.java  
  ${CMAKE_HOME_DIRECTORY}/examples/async/Forwarder.java
  ${CMAKE_HOME_DIRECTORY}/examples/async/Master.java  
  ${CMAKE_HOME_DIRECTORY}/examples/async/Slave.java
  ${CMAKE_HOME_DIRECTORY}/examples/bittorrent/Bittorrent.java
  ${CMAKE_HOME_DIRECTORY}/examples/bittorrent/Common.java
  ${CMAKE_HOME_DIRECTORY}/examples/bittorrent/Connection.java
  ${CMAKE_HOME_DIRECTORY}/examples/bittorrent/MessageTask.java
  ${CMAKE_HOME_DIRECTORY}/examples/bittorrent/Peer.java
  ${CMAKE_HOME_DIRECTORY}/examples/bittorrent/Tracker.java
  ${CMAKE_HOME_DIRECTORY}/examples/bittorrent/TrackerTask.java
  ${CMAKE_HOME_DIRECTORY}/examples/chord/Chord.java
  ${CMAKE_HOME_DIRECTORY}/examples/chord/Common.java
  ${CMAKE_HOME_DIRECTORY}/examples/chord/Node.java
  ${CMAKE_HOME_DIRECTORY}/examples/chord/ChordTask.java
  ${CMAKE_HOME_DIRECTORY}/examples/chord/FindSuccessorAnswerTask.java
  ${CMAKE_HOME_DIRECTORY}/examples/chord/FindSuccessorTask.java
  ${CMAKE_HOME_DIRECTORY}/examples/chord/GetPredecessorAnswerTask.java
  ${CMAKE_HOME_DIRECTORY}/examples/chord/NotifyTask.java
  ${CMAKE_HOME_DIRECTORY}/examples/commTime/FinalizeTask.java
  ${CMAKE_HOME_DIRECTORY}/examples/commTime/Master.java
  ${CMAKE_HOME_DIRECTORY}/examples/commTime/Slave.java
  ${CMAKE_HOME_DIRECTORY}/examples/commTime/CommTimeTest.java
  ${CMAKE_HOME_DIRECTORY}/examples/io/IO.java
  ${CMAKE_HOME_DIRECTORY}/examples/io/Node.java
  ${CMAKE_HOME_DIRECTORY}/examples/masterslave/FinalizeTask.java
  ${CMAKE_HOME_DIRECTORY}/examples/masterslave/Forwarder.java
  ${CMAKE_HOME_DIRECTORY}/examples/masterslave/Master.java
  ${CMAKE_HOME_DIRECTORY}/examples/masterslave/Masterslave.java
  ${CMAKE_HOME_DIRECTORY}/examples/masterslave/Slave.java	
  ${CMAKE_HOME_DIRECTORY}/examples/master_slave_bypass/Master.java
  ${CMAKE_HOME_DIRECTORY}/examples/master_slave_bypass/MsBypass.java
  ${CMAKE_HOME_DIRECTORY}/examples/master_slave_bypass/Slave.java
  ${CMAKE_HOME_DIRECTORY}/examples/master_slave_bypass/FinalizeTask.java
  ${CMAKE_HOME_DIRECTORY}/examples/master_slave_kill/Master.java
  ${CMAKE_HOME_DIRECTORY}/examples/master_slave_kill/MsKill.java
  ${CMAKE_HOME_DIRECTORY}/examples/master_slave_kill/Slave.java
  ${CMAKE_HOME_DIRECTORY}/examples/master_slave_kill/FinalizeTask.java
  ${CMAKE_HOME_DIRECTORY}/examples/migration/Emigrant.java
  ${CMAKE_HOME_DIRECTORY}/examples/migration/Migration.java
  ${CMAKE_HOME_DIRECTORY}/examples/migration/Policeman.java
  ${CMAKE_HOME_DIRECTORY}/examples/mutualExclusion/centralized/MutexCentral.java
  ${CMAKE_HOME_DIRECTORY}/examples/mutualExclusion/centralized/GrantTask.java
  ${CMAKE_HOME_DIRECTORY}/examples/mutualExclusion/centralized/Coordinator.java
  ${CMAKE_HOME_DIRECTORY}/examples/mutualExclusion/centralized/ReleaseTask.java
  ${CMAKE_HOME_DIRECTORY}/examples/mutualExclusion/centralized/Node.java
  ${CMAKE_HOME_DIRECTORY}/examples/mutualExclusion/centralized/RequestTask.java
  ${CMAKE_HOME_DIRECTORY}/examples/pingPong/PingPongTask.java
  ${CMAKE_HOME_DIRECTORY}/examples/pingPong/Sender.java
  ${CMAKE_HOME_DIRECTORY}/examples/pingPong/Receiver.java
  ${CMAKE_HOME_DIRECTORY}/examples/pingPong/PingPongTest.java
  ${CMAKE_HOME_DIRECTORY}/examples/startKillTime/Master.java
  ${CMAKE_HOME_DIRECTORY}/examples/startKillTime/Slave.java
  ${CMAKE_HOME_DIRECTORY}/examples/startKillTime/StartKillTime.java
  ${CMAKE_HOME_DIRECTORY}/examples/tracing/PingPongTask.java
  ${CMAKE_HOME_DIRECTORY}/examples/suspend/DreamMaster.java
  ${CMAKE_HOME_DIRECTORY}/examples/suspend/LazyGuy.java
  ${CMAKE_HOME_DIRECTORY}/examples/suspend/Suspend.java
  ${CMAKE_HOME_DIRECTORY}/examples/tracing/Sender.java
  ${CMAKE_HOME_DIRECTORY}/examples/tracing/Receiver.java
  ${CMAKE_HOME_DIRECTORY}/examples/tracing/TracingTest.java

)

set(CMAKE_SRC
  ${CMAKE_HOME_DIRECTORY}/ChangeLog
  ${CMAKE_HOME_DIRECTORY}/CMakeLists.txt
  ${CMAKE_HOME_DIRECTORY}/doxygen_postprocesser.pl
  ${CMAKE_HOME_DIRECTORY}/FindJavaSG.cmake
  ${CMAKE_HOME_DIRECTORY}/FindSimGrid.cmake
  ${CMAKE_HOME_DIRECTORY}/GenerateDoc.cmake
  ${CMAKE_HOME_DIRECTORY}/README
)

set(CMAKE_DOC_SRC
  ${CMAKE_HOME_DIRECTORY}/doc/Doxyfile.in
  ${CMAKE_HOME_DIRECTORY}/doc/footer.html.in
  ${CMAKE_HOME_DIRECTORY}/doc/index.doc
  ${CMAKE_HOME_DIRECTORY}/doc/install.doc
)

set(XML_FILES
  ${CMAKE_HOME_DIRECTORY}/examples/master_slave_bypass/platform.xml
  ${CMAKE_HOME_DIRECTORY}/examples/bittorrent/bittorrent.xml
  ${CMAKE_HOME_DIRECTORY}/examples/platform.xml
  ${CMAKE_HOME_DIRECTORY}/examples/commTime/commTimeDeployment.xml
  ${CMAKE_HOME_DIRECTORY}/examples/async/asyncDeployment.xml
  ${CMAKE_HOME_DIRECTORY}/examples/migration/migrationDeployment.xml
  ${CMAKE_HOME_DIRECTORY}/examples/masterslave/deploy.xml
  ${CMAKE_HOME_DIRECTORY}/examples/masterslave/masterslaveDeployment.xml
  ${CMAKE_HOME_DIRECTORY}/examples/chord/chord100.xml
  ${CMAKE_HOME_DIRECTORY}/examples/chord/chord.xml
  ${CMAKE_HOME_DIRECTORY}/examples/chord/chord10.xml
  ${CMAKE_HOME_DIRECTORY}/examples/chord/chord10000.xml
  ${CMAKE_HOME_DIRECTORY}/examples/chord/chord1000.xml
  ${CMAKE_HOME_DIRECTORY}/examples/chord/chord90.xml
  ${CMAKE_HOME_DIRECTORY}/examples/suspend/suspendDeployment.xml
  ${CMAKE_HOME_DIRECTORY}/examples/master_slave_kill/platform.xml
  ${CMAKE_HOME_DIRECTORY}/examples/mutualExclusion/centralized/mutex_centralized_deployment.xml
  ${CMAKE_HOME_DIRECTORY}/examples/mutualExclusion/ring3.xml
  ${CMAKE_HOME_DIRECTORY}/examples/priority/priorityDeployment.xml
  ${CMAKE_HOME_DIRECTORY}/examples/pingPong/pingPongDeployment.xml
  ${CMAKE_HOME_DIRECTORY}/examples/startKillTime/deployment_kill.xml
  ${CMAKE_HOME_DIRECTORY}/examples/startKillTime/deployment_start.xml
  ${CMAKE_HOME_DIRECTORY}/examples/startKillTime/deployment_start_kill.xml
  ${CMAKE_HOME_DIRECTORY}/examples/startKillTime/deployment.xml
)

set(TESH_FILES
  ${CMAKE_HOME_DIRECTORY}/examples/master_slave_bypass/bypass.tesh
  ${CMAKE_HOME_DIRECTORY}/examples/bittorrent/bittorrent.tesh
  ${CMAKE_HOME_DIRECTORY}/examples/commTime/commtime.tesh
  ${CMAKE_HOME_DIRECTORY}/examples/async/async.tesh
  ${CMAKE_HOME_DIRECTORY}/examples/migration/migration.tesh
  ${CMAKE_HOME_DIRECTORY}/examples/masterslave/masterslave.tesh
  ${CMAKE_HOME_DIRECTORY}/examples/chord/chord.tesh
  ${CMAKE_HOME_DIRECTORY}/examples/suspend/suspend.tesh
  ${CMAKE_HOME_DIRECTORY}/examples/master_slave_kill/kill.tesh
  ${CMAKE_HOME_DIRECTORY}/examples/mutualExclusion/mutualexclusion.tesh
  ${CMAKE_HOME_DIRECTORY}/examples/priority/priority.tesh
  ${CMAKE_HOME_DIRECTORY}/examples/pingPong/pingpong.tesh
  ${CMAKE_HOME_DIRECTORY}/examples/startKillTime/startKillTime.tesh
)

set(source_to_pack
  ${CMAKE_SRC}
  ${JMSG_C_SRC}
  ${JTRACE_C_SRC}
  ${JMSG_JAVA_SRC}
  ${JTRACE_JAVA_SRC}
  ${JAVA_EXAMPLES}
  ${XML_FILES}
  ${CMAKE_DOC_SRC}
  ${TESH_FILES}
)

string(REPLACE "org/" "classes/org/" JMSG_JAVA_SRC_CLASS "${JMSG_JAVA_SRC}")
string(REPLACE ".java" ".class" JMSG_JAVA_SRC_CLASS "${JMSG_JAVA_SRC_CLASS}")

string(REPLACE "org/" "classes/org/" JTRACE_JAVA_SRC_CLASS "${JTRACE_JAVA_SRC}")
string(REPLACE ".java" ".class" JTRACE_JAVA_SRC_CLASS "${JTRACE_JAVA_SRC_CLASS}")

string(REPLACE ".java" ".class" JAVA_EXAMPLES_CLASS "${JAVA_EXAMPLES}")

#########
# FLAGS #
#########
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g -Wall -Wunused -Wmissing-prototypes -Wmissing-declarations -Wpointer-arith -Wchar-subscripts -Wcomment -Wformat -Wwrite-strings -Wno-unused-function -Wno-unused-parameter -Wno-strict-aliasing -Wno-format-nonliteral -Werror ")
set(INCLUDE_PATH "-I${CMAKE_HOME_DIRECTORY}/src -I${SIMGRID_INCLUDES} ")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${INCLUDE_PATH}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error=int-to-pointer-cast -Wno-error=pointer-to-int-cast")
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
  set_target_properties(SG_java PROPERTIES LINK_FLAGS "-L${SIMGRID_LIB_PATH}")
endif(WIN32)

# SG_java_tracing
add_library(SG_java_tracing SHARED ${JTRACE_C_SRC})
if(WIN32)
  set_target_properties(SG_java_tracing PROPERTIES LINK_FLAGS "-Wl,--subsystem,windows,--kill-at ${SIMGRID_LIB}" PREFIX "")
  find_path(PEXPORTS_PATH NAMES pexports.exe PATHS NO_DEFAULT_PATHS)
  message(STATUS "pexports: ${PEXPORTS_PATH}")
  if(PEXPORTS_PATH)
    add_custom_command(TARGET SG_java_tracing POST_BUILD
    COMMAND ${PEXPORTS_PATH}/pexports.exe ${CMAKE_BINARY_DIR}/SG_java_tracing.dll > ${CMAKE_BINARY_DIR}/SG_java_tracing.def)
  endif(PEXPORTS_PATH)
else(WIN32)
  target_link_libraries(SG_java_tracing pcre pthread simgrid)
  set_target_properties(SG_java_tracing PROPERTIES LINK_FLAGS "-L${SIMGRID_LIB_PATH}")
endif(WIN32)


# java_classes
add_custom_command(
  OUTPUT  ${JMSG_JAVA_SRC_CLASS}
  DEPENDS ${JMSG_JAVA_SRC}
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_HOME_DIRECTORY}/classes/"
  COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/classes/
    -cp ${CMAKE_HOME_DIRECTORY}/classes/ ${JMSG_JAVA_SRC}
  COMMENT "Compiling java sources of core library..."					
)

add_custom_command(
  OUTPUT  ${JTRACE_JAVA_SRC_CLASS}
  DEPENDS ${JTRACE_JAVA_SRC}
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_HOME_DIRECTORY}/classes/"
  COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/classes/
    -cp ${CMAKE_HOME_DIRECTORY}/classes/ ${JAVA_FILES_TRACE}
  COMMENT "Compiling java sources of core libraries..."					
)

add_custom_target(java_classes ALL
  DEPENDS ${JMSG_JAVA_SRC_CLASS} ${JTRACE_JAVA_SRC_CLASS}
)

# simgrid_jar
add_custom_target(simgrid_jar ALL
  COMMAND ${JAVA_ARCHIVE} -cvf ${CMAKE_HOME_DIRECTORY}/simgrid.jar .
  COMMENT "Building simgrid.jar..."
  WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/classes 
)

# simgrid_java_examples
set(JAVA_EXAMPLES_CLASS
  ${JAVA_EXAMPLES_CLASS}
  ${CMAKE_HOME_DIRECTORY}/examples/bittorrent/MessageTask$Type.class
  ${CMAKE_HOME_DIRECTORY}/examples/bittorrent/Peer$1.class
  ${CMAKE_HOME_DIRECTORY}/examples/bittorrent/TrackerTask$Type.class
  ${CMAKE_HOME_DIRECTORY}/examples/chord/GetPredecessorTask.class
  ${CMAKE_HOME_DIRECTORY}/examples/priority/Test.class
  ${CMAKE_HOME_DIRECTORY}/examples/priority/Priority.class
)

add_custom_command(
  COMMENT "Build examples for java"
  OUTPUT  ${JAVA_EXAMPLES_CLASS}
  DEPENDS ${JAVA_EXAMPLES}
  COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/async/*.java
  COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/bittorrent/*.java
  COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/chord/*.java
  COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/commTime/*.java
  COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/io/*.java
  COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/masterslave/*.java
  COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/master_slave_bypass/*.java
  COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/master_slave_kill/*.java
  COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/migration/*.java
  COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/mutualExclusion/centralized/*.java              			
  COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/tracing/*.java
  COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/pingPong/*.java
  COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/priority/*.java   	
  COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/startKillTime/*.java
  COMMAND ${JAVA_COMPILE} -d ${CMAKE_HOME_DIRECTORY}/examples -cp ${CMAKE_HOME_DIRECTORY}/simgrid.jar ${CMAKE_HOME_DIRECTORY}/examples/suspend/*.java
)

add_custom_target(simgrid_java_examples ALL
  DEPENDS ${JAVA_EXAMPLES_CLASS}
)

########################
# Declare dependencies #
########################
add_dependencies(simgrid_jar java_classes)
add_dependencies(simgrid_java_examples simgrid_jar SG_java SG_java_tracing)

#####################
# Declare our tests #
#####################
INCLUDE(CTest)
ENABLE_TESTING()

ADD_TEST(async           ${TESH_BIN_PATH} ${TESH_OPTION} --setenv srcdir=${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/examples/async/async.tesh)
ADD_TEST(bittorrent      ${TESH_BIN_PATH} ${TESH_OPTION} --setenv srcdir=${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/examples/bittorrent/bittorrent.tesh)
ADD_TEST(bypass          ${TESH_BIN_PATH} ${TESH_OPTION} --setenv srcdir=${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/examples/master_slave_bypass/bypass.tesh)
ADD_TEST(commTime        ${TESH_BIN_PATH} ${TESH_OPTION} --setenv srcdir=${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/examples/commTime/commtime.tesh)
ADD_TEST(chord        	 ${TESH_BIN_PATH} ${TESH_OPTION} --setenv srcdir=${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/examples/chord/chord.tesh)
ADD_TEST(kill            ${TESH_BIN_PATH} ${TESH_OPTION} --setenv srcdir=${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/examples/master_slave_kill/kill.tesh)
ADD_TEST(masterslave     ${TESH_BIN_PATH} ${TESH_OPTION} --setenv srcdir=${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/examples/masterslave/masterslave.tesh)
ADD_TEST(migration       ${TESH_BIN_PATH} ${TESH_OPTION} --setenv srcdir=${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/examples/migration/migration.tesh)
ADD_TEST(mutualExclusion ${TESH_BIN_PATH} ${TESH_OPTION} --setenv srcdir=${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/examples/mutualExclusion/mutualexclusion.tesh)
ADD_TEST(pingPong        ${TESH_BIN_PATH} ${TESH_OPTION} --setenv srcdir=${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/examples/pingPong/pingpong.tesh)
ADD_TEST(priority        ${TESH_BIN_PATH} ${TESH_OPTION} --setenv srcdir=${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/examples/priority/priority.tesh)
ADD_TEST(startKillTime   ${TESH_BIN_PATH} ${TESH_OPTION} --setenv srcdir=${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/examples/startKillTime/startKillTime.tesh)
ADD_TEST(suspend         ${TESH_BIN_PATH} ${TESH_OPTION} --setenv srcdir=${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/examples/suspend/suspend.tesh)
ADD_TEST(tracing         ${TESH_BIN_PATH} ${TESH_OPTION} --setenv srcdir=${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/examples/tracing/tracingPingPong.tesh)
#Don't forget to put new test in this list!!!
set(test_list async bittorrent bypass chord commTime kill masterslave migration mutualExclusion pingPong priority startKillTime tracing)

##########################################
# Set the  DYLD_LIBRARY_PATH for mac     #
##########################################
if(APPLE)
  if($ENV{DYLD_LIBRARY_PATH})
    set_tests_properties(${test_list}
    PROPERTIES ENVIRONMENT "DYLD_LIBRARY_PATH=${CMAKE_HOME_DIRECTORY}/lib:${SIMGRID_LIB_PATH}")
  else($ENV{DYLD_LIBRARY_PATH})
    set_tests_properties(${test_list}
    PROPERTIES ENVIRONMENT "DYLD_LIBRARY_PATH=$ENV{DYLD_LIBRARY_PATH}:${CMAKE_HOME_DIRECTORY}/lib:${SIMGRID_LIB_PATH}")
  endif($ENV{DYLD_LIBRARY_PATH})
else(APPLE)
##########################################
# Set the  LD_LIBRARY_PATH for our tests #
##########################################
  if(NOT WIN32)
    if($ENV{LD_LIBRARY_PATH})
      set_tests_properties(${test_list}
      PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_HOME_DIRECTORY}/lib:${SIMGRID_LIB_PATH}")
    else($ENV{LD_LIBRARY_PATH})
      set_tests_properties(${test_list}
      PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=$ENV{LD_LIBRARY_PATH}:${CMAKE_HOME_DIRECTORY}/lib:${SIMGRID_LIB_PATH}")
    endif($ENV{LD_LIBRARY_PATH})
  endif(NOT WIN32)
endif(APPLE)

###########
# Install #
###########
install(TARGETS SG_java
  DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/java/)
  
install(TARGETS SG_java_tracing
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
  COMMAND ${CMAKE_COMMAND} -E make_directory ${DIST_NAME}/doc/html/
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_HOME_DIRECTORY}/doc/html/ ${DIST_NAME}/doc/html/
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

##########################
# Extra make clean files #
##########################
SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
  ${CMAKE_HOME_DIRECTORY}/simgrid.jar
)

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
