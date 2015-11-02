##
## The Cmake definitions for the use of Java (and Scala)
##   This file is loaded only if the Java option is activated
##

find_package(Java 1.7 REQUIRED)
include(UseJava)

# Rules to build libsimgrid-java
#
add_library(simgrid-java SHARED ${JMSG_C_SRC})
set_target_properties(simgrid-java PROPERTIES VERSION ${libsimgrid-java_version})

get_target_property(COMMON_INCLUDES simgrid-java INCLUDE_DIRECTORIES)
if (COMMON_INCLUDES)
  set_target_properties(simgrid-java PROPERTIES
    INCLUDE_DIRECTORIES "${COMMON_INCLUDES};${JNI_INCLUDE_DIRS}")
else()
  set_target_properties(simgrid-java PROPERTIES
    INCLUDE_DIRECTORIES "${JNI_INCLUDE_DIRS}")
endif()

get_target_property(CHECK_INCLUDES simgrid-java INCLUDE_DIRECTORIES)
message("-- [Java] simgrid-java includes: ${CHECK_INCLUDES}")

target_link_libraries(simgrid-java simgrid)


if(WIN32)
  exec_program("java -d32 -version" OUTPUT_VARIABLE IS_32_BITS_JVM)
  STRING( FIND ${IS_32_BITS_JVM} "Error" POSITION )
  if(NOT ${POSITION} GREATER -1)
    message(fatal_error "SimGrid can only use Java 64 bits")
  endif()
endif()

# Rules to build simgrid.jar
#

## Files to include in simgrid.jar
##
set(SIMGRID_JAR "${CMAKE_BINARY_DIR}/simgrid.jar")
set(MANIFEST_IN_FILE "${CMAKE_HOME_DIRECTORY}/src/bindings/java/MANIFEST.MF.in")
set(MANIFEST_FILE "${CMAKE_BINARY_DIR}/src/bindings/java/MANIFEST.MF")

set(LIBSIMGRID_SO
  libsimgrid${CMAKE_SHARED_LIBRARY_SUFFIX})
set(LIBSIMGRID_JAVA_SO
  ${CMAKE_SHARED_LIBRARY_PREFIX}simgrid-java${CMAKE_SHARED_LIBRARY_SUFFIX})
set(LIBSURF_JAVA_SO
  ${CMAKE_SHARED_LIBRARY_PREFIX}surf-java${CMAKE_SHARED_LIBRARY_SUFFIX})

## Here is how to build simgrid.jar
##
if(CMAKE_VERSION VERSION_LESS "2.8.12")
  set(CMAKE_JAVA_TARGET_OUTPUT_NAME simgrid)
  add_jar(simgrid-java_jar ${JMSG_JAVA_SRC})
else()
  add_jar(simgrid-java_jar ${JMSG_JAVA_SRC} OUTPUT_NAME simgrid)
endif()

add_custom_command(
  TARGET simgrid-java_jar POST_BUILD
  COMMENT "Add the documentation into simgrid.jar..."
  DEPENDS ${MANIFEST_IN_FILE}
	  ${CMAKE_HOME_DIRECTORY}/COPYING
	  ${CMAKE_HOME_DIRECTORY}/ChangeLog
	  ${CMAKE_HOME_DIRECTORY}/NEWS
	  ${CMAKE_HOME_DIRECTORY}/ChangeLog.SimGrid-java
	  ${CMAKE_HOME_DIRECTORY}/LICENSE-LGPL-2.1
	    
  COMMAND ${CMAKE_COMMAND} -E copy ${MANIFEST_IN_FILE} ${MANIFEST_FILE}
  COMMAND ${CMAKE_COMMAND} -E echo "Specification-Version: \\\"${SIMGRID_VERSION_MAJOR}.${SIMGRID_VERSION_MINOR}.${SIMGRID_VERSION_PATCH}\\\"" >> ${MANIFEST_FILE}
  COMMAND ${CMAKE_COMMAND} -E echo "Implementation-Version: \\\"${GIT_VERSION}\\\"" >> ${MANIFEST_FILE}

  COMMAND ${Java_JAVADOC_EXECUTABLE} -quiet -d doc/javadoc -sourcepath ${CMAKE_HOME_DIRECTORY}/src/bindings/java/ org.simgrid.msg org.simgrid.surf org.simgrid.trace
  
  COMMAND ${JAVA_ARCHIVE} -uvmf ${MANIFEST_FILE} ${SIMGRID_JAR} doc/javadoc -C ${CMAKE_HOME_DIRECTORY} COPYING ChangeLog ChangeLog.SimGrid-java LICENSE-LGPL-2.1 NEWS
  )

###
### Pack the java libraries into the jarfile if asked to do so
###

if(enable_lib_in_jar)
  find_program(STRIP_COMMAND strip)
  mark_as_advanced(STRIP_COMMAND)
  if(NOT STRIP_COMMAND)
    set(STRIP_COMMAND "cmake -E echo (strip not found)")
  endif()
  set(SG_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})
  
  if(${SG_SYSTEM_NAME} MATCHES "kFreeBSD")
    set(SG_SYSTEM_NAME GNU/kFreeBSD)
  endif()

  set(JAVA_NATIVE_PATH NATIVE/${SG_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR})
  if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "^i[3-6]86$")
    set(JAVA_NATIVE_PATH NATIVE/${SG_SYSTEM_NAME}/x86)
  endif()
  if(  (${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64") OR
       (${CMAKE_SYSTEM_PROCESSOR} MATCHES "AMD64")     )
    set(JAVA_NATIVE_PATH NATIVE/${SG_SYSTEM_NAME}/amd64)
  endif()
  if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "armv7l")
    set(JAVA_NATIVE_PATH NATIVE/${SG_SYSTEM_NAME}/arm) # Default arm (soft-float ABI)
  endif()

  add_custom_command(
    TARGET simgrid-java_jar POST_BUILD
    COMMENT "Add the native libs into simgrid.jar..."
    DEPENDS simgrid-java 
	    ${CMAKE_BINARY_DIR}/lib/${LIBSIMGRID_SO}
	    ${CMAKE_BINARY_DIR}/lib/${LIBSIMGRID_JAVA_SO}
	    ${CMAKE_BINARY_DIR}/lib/${LIBSURF_JAVA_SO}
	  
    COMMAND ${CMAKE_COMMAND} -E remove_directory NATIVE
    COMMAND ${CMAKE_COMMAND} -E make_directory                                     ${JAVA_NATIVE_PATH}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/lib/${LIBSIMGRID_SO}      ${JAVA_NATIVE_PATH}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/lib/${LIBSIMGRID_JAVA_SO} ${JAVA_NATIVE_PATH}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/lib/${LIBSURF_JAVA_SO}    ${JAVA_NATIVE_PATH}
    
    # strip seems to fail on Mac on binaries that are already stripped.
    # It then spits: "symbols referenced by indirect symbol table entries that can't be stripped"
    COMMAND ${STRIP_COMMAND} ${JAVA_NATIVE_PATH}/${LIBSIMGRID_SO}      || true
    COMMAND ${STRIP_COMMAND} ${JAVA_NATIVE_PATH}/${LIBSIMGRID_JAVA_SO} || true
    COMMAND ${STRIP_COMMAND} ${JAVA_NATIVE_PATH}/${LIBSURF_JAVA_SO}    || true

    COMMAND ${JAVA_ARCHIVE} -uvf ${SIMGRID_JAR}  NATIVE
    COMMAND ${CMAKE_COMMAND} -E remove_directory NATIVE
    
    COMMAND ${CMAKE_COMMAND} -E echo "-- Cmake put the native code in ${JAVA_NATIVE_PATH}"
    COMMAND "${Java_JAVA_EXECUTABLE}" -classpath "${SIMGRID_JAR}" org.simgrid.NativeLib
    )
  if(MINGW)
    find_library(WINPTHREAD_DLL
      NAME winpthread winpthread-1
      PATHS C:\\MinGW C:\\MinGW64 C:\\MinGW\\bin C:\\MinGW64\\bin
    )
    add_custom_command(
      TARGET simgrid-java_jar POST_BUILD
      COMMENT "Add the MinGW libs into simgrid.jar..."
      DEPENDS ${CMAKE_BINARY_DIR}/lib/${LIBSIMGRID_SO}

      COMMAND ${CMAKE_COMMAND} -E remove_directory NATIVE
      COMMAND ${CMAKE_COMMAND} -E make_directory          ${JAVA_NATIVE_PATH}
      COMMAND ${CMAKE_COMMAND} -E copy ${WINPTHREAD_DLL}  ${JAVA_NATIVE_PATH}

      COMMAND ${JAVA_ARCHIVE} -uvf ${SIMGRID_JAR}  NATIVE
      COMMAND ${CMAKE_COMMAND} -E remove_directory NATIVE
    )
  endif(MINGW)
endif(enable_lib_in_jar)

include_directories(${JNI_INCLUDE_DIRS} ${JAVA_INCLUDE_PATH} ${JAVA_INCLUDE_PATH2})

if(SWIG_FOUND)
  set(CMAKE_SWIG_FLAGS "-package" "org.simgrid.surf")
  set(CMAKE_SWIG_OUTDIR "${CMAKE_HOME_DIRECTORY}/src/bindings/java/org/simgrid/surf")

  set_source_files_properties(${JSURF_SWIG_SRC} PROPERTIES CPLUSPLUS 1)

  swig_add_module(surf-java java ${JSURF_SWIG_SRC} ${JSURF_JAVA_C_SRC})
  swig_link_libraries(surf-java simgrid)
else()
  add_library(surf-java SHARED ${JSURF_C_SRC})
  target_link_libraries(surf-java simgrid)
endif()

set_target_properties(surf-java PROPERTIES SKIP_BUILD_RPATH ON)
set_target_properties(simgrid-java PROPERTIES SKIP_BUILD_RPATH ON)

add_dependencies(simgrid-java surf-java)
add_dependencies(simgrid-java_jar surf-java)

