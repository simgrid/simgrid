##
## The Cmake definitions for the use of Java
##
## This file is loaded only if the Java option is activated, and Java+JNI are found
##

include(UseJava)

# Generate the native part of the bindings
##########################################
if(NOT ${merge_java_in_libsimgrid})
  message(STATUS "Java: build a split libsimgrid-java library.") 
  add_library(simgrid-java SHARED
              ${CMAKE_CURRENT_BINARY_DIR}/include/org_simgrid_s4u_simgridJNI.h
              ${SIMGRID_JAVA_C_SOURCES})
  set_property(TARGET simgrid-java
               APPEND PROPERTY INCLUDE_DIRECTORIES ${JNI_INCLUDE_DIRS} "${INTERNAL_INCLUDES}")
  set_target_properties(simgrid-java PROPERTIES VERSION ${libsimgrid-java_version})
  set_target_properties(simgrid-java PROPERTIES SKIP_BUILD_RPATH ON)
  target_link_libraries(simgrid-java PUBLIC simgrid)
  add_dependencies(tests simgrid-java)

  get_target_property(CHECK_INCLUDES simgrid-java INCLUDE_DIRECTORIES)
  message(STATUS "[Java] simgrid-java includes: ${CHECK_INCLUDES}")
endif()

# Generate the header file ensuring that the C++ and Java versions of the JNI bindings match
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include/org_simgrid_s4u_simgridJNI.h
                  COMMAND javac -cp ${CMAKE_HOME_DIRECTORY}/src/bindings/java/ -h ${CMAKE_CURRENT_BINARY_DIR}/include/ ${CMAKE_HOME_DIRECTORY}/src/bindings/java/org/simgrid/s4u/simgridJNI.java
                  DEPENDS ${CMAKE_HOME_DIRECTORY}/src/bindings/java/org/simgrid/s4u/simgridJNI.java)

# Rules to build simgrid.jar
############################

## Files to include in simgrid.jar
## 
set(SIMGRID_JAR "${CMAKE_BINARY_DIR}/simgrid.jar")
set(MANIFEST_IN_FILE "${CMAKE_HOME_DIRECTORY}/src/bindings/java/MANIFEST.in")
set(MANIFEST_FILE "${CMAKE_BINARY_DIR}/src/bindings/java/MANIFEST.MF")

if(${merge_java_in_libsimgrid})
  set(LIBSIMGRID_SO  ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}simgrid${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
  set(LIBSIMGRID_SO  ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}simgrid${CMAKE_SHARED_LIBRARY_SUFFIX} 
                     ${CMAKE_BINARY_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}simgrid-java${CMAKE_SHARED_LIBRARY_SUFFIX})
endif()
set(LIBSIMGRID_JAVA_SO  )

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
  if(${merge_java_in_libsimgrid})
    add_dependencies(simgrid_jar simgrid)
    set(JAVALIBS simgrid)
  else()
    add_dependencies(simgrid_jar simgrid-java)
    add_dependencies(simgrid_jar simgrid)
    set(JAVALIBS simgrid simgrid-java)
  endif()

  add_custom_target(NATIVE
    COMMENT "Add the native libs into simgrid.jar..."
    DEPENDS ${JAVALIBS}

    COMMAND ${CMAKE_COMMAND} -E make_directory   ${JAVA_NATIVE_PATH}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIBSIMGRID_SO}      ${JAVA_NATIVE_PATH}
    COMMAND ${CMAKE_STRIP} -x ${JAVA_NATIVE_PATH}/*
    )
endif()

add_custom_target(java-bindings COMMENT "Recompiling the Java files: jarfile and libraries (use 'tests-java' to get the Java tests in addition).")

add_custom_target(tests-java COMMENT "Building all Java examples...")
add_dependencies(tests tests-java)
add_dependencies(tests-java java-bindings)

add_dependencies(java-bindings simgrid_jar)
add_dependencies(java-bindings simgrid)      # useful when the libs are not included in the jar
if(NOT ${merge_java_in_libsimgrid})
  add_dependencies(java-bindings simgrid-java) # useful when the libs are not included in the jar
endif()

include_directories(${JNI_INCLUDE_DIRS} ${JAVA_INCLUDE_PATH} ${JAVA_INCLUDE_PATH2})
