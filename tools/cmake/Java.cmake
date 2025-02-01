##
## The Cmake definitions for the use of Java
##
## This file is loaded only if the Java option is activated, and Java+JNI are found
##

include(UseJava)

# Generate the native part of the bindings
##########################################
add_library(simgrid-java SHARED
            ${CMAKE_CURRENT_BINARY_DIR}/include/org_simgrid_s4u_simgridJNI.h
            ${SIMGRID_JAVA_C_SOURCES})
set_property(TARGET simgrid-java
             APPEND PROPERTY INCLUDE_DIRECTORIES ${JNI_INCLUDE_DIRS} "${INTERNAL_INCLUDES}")
set_target_properties(simgrid-java PROPERTIES VERSION ${libsimgrid-java_version})
set_target_properties(simgrid-java PROPERTIES SKIP_BUILD_RPATH ON)
target_link_libraries(simgrid-java PUBLIC simgrid)
add_dependencies(tests simgrid-java)

# Generate the header file ensuring that the C++ and Java versions of the JNI bindings match
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include/org_simgrid_s4u_simgridJNI.h
                  COMMAND javac -cp ${CMAKE_HOME_DIRECTORY}/src/bindings/java/ -h ${CMAKE_CURRENT_BINARY_DIR}/include/ ${CMAKE_HOME_DIRECTORY}/src/bindings/java/org/simgrid/s4u/simgridJNI.java
                  DEPENDS ${CMAKE_HOME_DIRECTORY}/src/bindings/java/org/simgrid/s4u/simgridJNI.java)

get_target_property(CHECK_INCLUDES simgrid-java INCLUDE_DIRECTORIES)
message(STATUS "[Java] simgrid-java includes: ${CHECK_INCLUDES}")

# Rules to build simgrid.jar
############################

## Files to include in simgrid.jar
## 
set(SIMGRID_JAR "${CMAKE_BINARY_DIR}/simgrid.jar")
set(MANIFEST_IN_FILE "${CMAKE_HOME_DIRECTORY}/src/bindings/java/MANIFEST.in")
set(MANIFEST_FILE "${CMAKE_BINARY_DIR}/src/bindings/java/MANIFEST.MF")

set(LIBSIMGRID_SO       ${CMAKE_SHARED_LIBRARY_PREFIX}simgrid${CMAKE_SHARED_LIBRARY_SUFFIX})
set(LIBSIMGRID_JAVA_SO  ${CMAKE_SHARED_LIBRARY_PREFIX}simgrid-java${CMAKE_SHARED_LIBRARY_SUFFIX})

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


## Build simgrid.jar. 
##
add_jar(simgrid_jar
        SOURCES ${SIMGRID_JAVA_JAVA_SOURCES}
        OUTPUT_NAME simgrid)

if (enable_lib_in_jar)
# Add the classes of the generated sources later, as they do not exist at configure time when add_jar computes its arguments
ADD_CUSTOM_COMMAND(TARGET simgrid_jar 
        COMMENT "Pack the native code into simgrid.jar if asked to (not doing so speedups builds when hacking on Java)."
        POST_BUILD
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND set -e
        COMMAND ${CMAKE_MAKE_PROGRAM} NATIVE
        COMMAND ${JAVA_ARCHIVE} -uvf ${SIMGRID_JAR} NATIVE
        COMMAND ${CMAKE_COMMAND} -E echo "-- CMake puts the native code in ${JAVA_NATIVE_PATH}"
        COMMAND "${Java_JAVA_EXECUTABLE}" -classpath "${SIMGRID_JAR}" org.simgrid.s4u.NativeLib
    )
endif()

if (enable_documentation)
  add_custom_command(
    TARGET simgrid_jar POST_BUILD
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
  add_dependencies(simgrid_jar simgrid-java)
  add_dependencies(simgrid_jar simgrid)

  add_custom_target(NATIVE
    COMMENT "Add the native libs into simgrid.jar..."
    DEPENDS simgrid simgrid-java ${JAVALIBS}

    COMMAND ${CMAKE_COMMAND} -E make_directory   ${JAVA_NATIVE_PATH}

    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/lib/${LIBSIMGRID_SO}      ${JAVA_NATIVE_PATH}/${LIBSIMGRID_SO}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/lib/${LIBSIMGRID_JAVA_SO} ${JAVA_NATIVE_PATH}/${LIBSIMGRID_JAVA_SO}
    )

  if(APPLE)
    add_custom_command(
      TARGET simgrid_jar POST_BUILD
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

else()
#  add_custom_target(NATIVE
#    COMMENT "Faking the addition of native libraries in the jar, as requested by enable_lib_in_jar"
#    COMMAND rm -rf NATIVE
#    COMMAND mkdir NATIVE
#  )

endif(enable_lib_in_jar)

add_dependencies(tests simgrid_jar)

include_directories(${JNI_INCLUDE_DIRS} ${JAVA_INCLUDE_PATH} ${JAVA_INCLUDE_PATH2})
