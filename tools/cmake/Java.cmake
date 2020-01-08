##
## The Cmake definitions for the use of Java (and Scala)
##   This file is loaded only if the Java option is activated
##

find_package(Java 1.8 COMPONENTS Runtime Development)
if (NOT ${Java_FOUND})
  message(FATAL_ERROR "Java not found (need at least Java8). Please install the JDK or disable that option")
endif()
set(Java_FOUND 1)
include(UseJava)

find_package(JNI REQUIRED)
message(STATUS "[Java] JNI found: ${JNI_FOUND}")
message(STATUS "[Java] JNI include dirs: ${JNI_INCLUDE_DIRS}")

if(WIN32)
  execute_process(COMMAND         java -d64 -version
                  OUTPUT_VARIABLE JVM_IS_64_BITS)
  if("${JVM_IS_64_BITS}" MATCHES "Error")
    message(fatal_error "SimGrid can only use Java 64 bits")
  endif()
endif()

# Rules to build libsimgrid-java
################################

add_library(simgrid-java SHARED ${JMSG_C_SRC})
set_target_properties(simgrid-java PROPERTIES VERSION          ${libsimgrid-java_version})
set_target_properties(simgrid-java PROPERTIES SKIP_BUILD_RPATH ON)
set_property(TARGET simgrid-java
             APPEND PROPERTY INCLUDE_DIRECTORIES "${INTERNAL_INCLUDES}")

target_link_libraries(simgrid-java simgrid)
add_dependencies(tests simgrid-java)

get_target_property(COMMON_INCLUDES simgrid-java INCLUDE_DIRECTORIES)
if (COMMON_INCLUDES)
  set_target_properties(simgrid-java PROPERTIES  INCLUDE_DIRECTORIES "${COMMON_INCLUDES};${JNI_INCLUDE_DIRS}")
else()
  set_target_properties(simgrid-java PROPERTIES  INCLUDE_DIRECTORIES "${JNI_INCLUDE_DIRS}")
endif()

get_target_property(CHECK_INCLUDES simgrid-java INCLUDE_DIRECTORIES)
message(STATUS "[Java] simgrid-java includes: ${CHECK_INCLUDES}")

# Rules to build simgrid.jar
############################

## Files to include in simgrid.jar
##
set(SIMGRID_JAR "${CMAKE_BINARY_DIR}/simgrid.jar")
set(MANIFEST_IN_FILE "${CMAKE_HOME_DIRECTORY}/src/bindings/java/MANIFEST.in")
set(MANIFEST_FILE "${CMAKE_BINARY_DIR}/src/bindings/java/MANIFEST.MF")

set(LIBSIMGRID_SO       libsimgrid${CMAKE_SHARED_LIBRARY_SUFFIX})
set(LIBSIMGRID_JAVA_SO  ${CMAKE_SHARED_LIBRARY_PREFIX}simgrid-java${CMAKE_SHARED_LIBRARY_SUFFIX})

## Here is how to build simgrid.jar
##
if(CMAKE_VERSION VERSION_LESS "2.8.12")
  set(CMAKE_JAVA_TARGET_OUTPUT_NAME simgrid)
  add_jar(simgrid-java_jar ${JMSG_JAVA_SRC})
else()
  add_jar(simgrid-java_jar ${JMSG_JAVA_SRC} OUTPUT_NAME simgrid)
endif()

if(enable_lib_in_jar)
  add_dependencies(simgrid-java_jar simgrid-java)
  add_dependencies(simgrid-java_jar simgrid)
endif()

if (enable_documentation)
  add_custom_command(
    TARGET simgrid-java_jar POST_BUILD
    COMMENT "Add the documentation into simgrid.jar..."
    DEPENDS ${MANIFEST_IN_FILE}
            ${CMAKE_HOME_DIRECTORY}/COPYING
            ${CMAKE_HOME_DIRECTORY}/ChangeLog
            ${CMAKE_HOME_DIRECTORY}/NEWS
            ${CMAKE_HOME_DIRECTORY}/LICENSE-LGPL-2.1

    COMMAND ${CMAKE_COMMAND} -E copy ${MANIFEST_IN_FILE} ${MANIFEST_FILE}
    COMMAND ${CMAKE_COMMAND} -E echo "Specification-Version: \\\"${SIMGRID_VERSION_MAJOR}.${SIMGRID_VERSION_MINOR}.${SIMGRID_VERSION_PATCH}\\\"" >> ${MANIFEST_FILE}
    COMMAND ${CMAKE_COMMAND} -E echo "Implementation-Version: \\\"${GIT_VERSION}\\\"" >> ${MANIFEST_FILE}

    COMMAND ${Java_JAVADOC_EXECUTABLE} -quiet -d doc/javadoc -sourcepath ${CMAKE_HOME_DIRECTORY}/src/bindings/java/ org.simgrid.msg org.simgrid.trace

    COMMAND ${JAVA_ARCHIVE} -uvmf ${MANIFEST_FILE} ${SIMGRID_JAR} doc/javadoc
    COMMAND ${JAVA_ARCHIVE} -uvf  ${SIMGRID_JAR} -C ${CMAKE_HOME_DIRECTORY} COPYING -C ${CMAKE_HOME_DIRECTORY} ChangeLog -C ${CMAKE_HOME_DIRECTORY} LICENSE-LGPL-2.1 -C ${CMAKE_HOME_DIRECTORY} NEWS
  )
endif()

###
### Pack the java libraries into the jarfile if asked to do so
###

if(enable_lib_in_jar)
  set(SG_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})

  if(${SG_SYSTEM_NAME} MATCHES "kFreeBSD")
    set(SG_SYSTEM_NAME GNU/kFreeBSD)
  endif()

  set(JAVA_NATIVE_PATH NATIVE/${SG_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR})
  if( (${CMAKE_SYSTEM_PROCESSOR} MATCHES "^i[3-6]86$") OR
      (${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64") OR
      (${CMAKE_SYSTEM_PROCESSOR} MATCHES "AMD64") )
    if(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32 bits
      set(JAVA_NATIVE_PATH NATIVE/${SG_SYSTEM_NAME}/x86)
    else()
      set(JAVA_NATIVE_PATH NATIVE/${SG_SYSTEM_NAME}/amd64)
    endif()
  endif()
  if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "armv7l")
    set(JAVA_NATIVE_PATH NATIVE/${SG_SYSTEM_NAME}/arm) # Default arm (soft-float ABI)
  endif()

  add_custom_command(
    TARGET simgrid-java_jar POST_BUILD
    COMMENT "Add the native libs into simgrid.jar..."
    DEPENDS simgrid simgrid-java ${JAVALIBS}
	
    COMMAND ${CMAKE_COMMAND} -E make_directory   ${JAVA_NATIVE_PATH}

    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/lib/${LIBSIMGRID_SO}      ${JAVA_NATIVE_PATH}/${LIBSIMGRID_SO}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/lib/${LIBSIMGRID_JAVA_SO} ${JAVA_NATIVE_PATH}/${LIBSIMGRID_JAVA_SO}
  )

if(WIN32)
  add_custom_command(
    TARGET simgrid-java_jar POST_BUILD
    COMMENT "Add the windows-specific native libs into simgrid.jar..."
    DEPENDS simgrid simgrid-java ${JAVALIBS}

    # There is no way to disable the dependency of mingw-64 on that lib, unfortunately nor to script cmake -E properly
    # So let's be brutal and copy it in any case (even on non-windows builds) from the location where appveyor provides it.
    # The copy is only expected to work on the appveyor builder, but that's all we need right now
    # since our users are directed to download that file as nightly build.
    COMMAND ${CMAKE_COMMAND} -E copy_if_different C:/mingw-w64/x86_64-7.2.0-posix-seh-rt_v5-rev1/mingw64/bin/libwinpthread-1.dll ${JAVA_NATIVE_PATH}/libwinpthread-1.dll || true
    COMMAND ${CMAKE_COMMAND} -E copy_if_different C:/ProgramData/chocolatey/lib/mingw/tools/install/mingw64/bin/libwinpthread-1.dll  ${JAVA_NATIVE_PATH}/libwinpthread-1.dll || true
  )
endif()

if(APPLE)
  add_custom_command(
    TARGET simgrid-java_jar POST_BUILD
    COMMENT "Add the apple-specific native libs into simgrid.jar..."
    DEPENDS simgrid simgrid-java ${JAVALIBS}

    # We need to fix the rpath of the simgrid-java library so that it
    #    searches the simgrid library in the right location
    #
    # Since we don't officially install the lib before copying it in
    # the jarfile, the lib is searched for where it was built. Given
    # how we unpack it, we need to instruct simgrid-java to forget
    # about the build path, and search in its current directory
    # instead.
    #
    # This has to be done with the classical Apple tools, as follows:

    COMMAND install_name_tool -change ${CMAKE_BINARY_DIR}/lib/libsimgrid.${SIMGRID_VERSION_MAJOR}.${SIMGRID_VERSION_MINOR}${CMAKE_SHARED_LIBRARY_SUFFIX} @loader_path/libsimgrid.dylib ${JAVA_NATIVE_PATH}/${LIBSIMGRID_JAVA_SO}
  )
endif(APPLE)

  add_custom_command(
    TARGET simgrid-java_jar POST_BUILD
    COMMENT "Packing back the simgrid.jar with the native libs..."
    DEPENDS simgrid simgrid-java ${JAVALIBS}

    COMMAND ${JAVA_ARCHIVE} -uvf ${SIMGRID_JAR}  ${JAVA_NATIVE_PATH}

    COMMAND ${CMAKE_COMMAND} -E echo "-- Cmake put the native code in ${JAVA_NATIVE_PATH}"
    COMMAND "${Java_JAVA_EXECUTABLE}" -classpath "${SIMGRID_JAR}" org.simgrid.NativeLib
  )

endif(enable_lib_in_jar)

include_directories(${JNI_INCLUDE_DIRS} ${JAVA_INCLUDE_PATH} ${JAVA_INCLUDE_PATH2})
