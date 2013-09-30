cmake_minimum_required(VERSION 2.8.6)

include(UseJava)

# Rules to build libSG_java
#
add_library(SG_java SHARED ${JMSG_C_SRC})
set_target_properties(SG_java PROPERTIES VERSION ${libSG_java_version})
if (CMAKE_VERSION VERSION_LESS "2.8.8")
  include_directories(${JNI_INCLUDE_DIRS})

  message(WARNING "[Java] Try to workaround missing feature in older CMake.  You should better update CMake to version 2.8.8 or above.")
  get_directory_property(CHECK_INCLUDES INCLUDE_DIRECTORIES)
else()
  get_target_property(COMMON_INCLUDES SG_java INCLUDE_DIRECTORIES)
  if (COMMON_INCLUDES)
    set_target_properties(SG_java PROPERTIES
      INCLUDE_DIRECTORIES "${COMMON_INCLUDES};${JNI_INCLUDE_DIRS}")
  else()
    set_target_properties(SG_java PROPERTIES
      INCLUDE_DIRECTORIES "${JNI_INCLUDE_DIRS}")
  endif()
  add_dependencies(SG_java simgrid)

  get_target_property(CHECK_INCLUDES SG_java INCLUDE_DIRECTORIES)
endif()
message("-- [Java] SG_java includes: ${CHECK_INCLUDES}")

target_link_libraries(SG_java simgrid)

if(WIN32)
  set_target_properties(SG_java PROPERTIES
    LINK_FLAGS "-Wl,--subsystem,windows,--kill-at"
    PREFIX "")
  find_path(PEXPORTS_PATH NAMES pexports.exe PATHS NO_DEFAULT_PATHS)
  message(STATUS "pexports: ${PEXPORTS_PATH}")
  if(PEXPORTS_PATH)
    add_custom_command(TARGET SG_java POST_BUILD
      COMMAND ${PEXPORTS_PATH}/pexports.exe ${CMAKE_BINARY_DIR}/lib/SG_java.dll > ${CMAKE_BINARY_DIR}/lib/SG_java.def)
  endif(PEXPORTS_PATH)
endif()

# Rules to build simgrid.jar
#

## Files to include in simgrid.jar
##
set(SIMGRID_JAR "${CMAKE_BINARY_DIR}/simgrid.jar")
set(MANIFEST_FILE "${CMAKE_HOME_DIRECTORY}/src/bindings/java/MANIFEST.MF")
set(LIBSIMGRID_SO
  ${CMAKE_SHARED_LIBRARY_PREFIX}simgrid${CMAKE_SHARED_LIBRARY_SUFFIX})
set(LIBSG_JAVA_SO
  ${CMAKE_SHARED_LIBRARY_PREFIX}SG_java${CMAKE_SHARED_LIBRARY_SUFFIX})

## Name of the "NATIVE" folder in simgrid.jar
##
if(CMAKE_SYSTEM_PROCESSOR MATCHES ".86")
  if(${ARCH_32_BITS})
    set(JSG_BUNDLE "NATIVE/${CMAKE_SYSTEM_NAME}/i386/")
  else()
    set(JSG_BUNDLE "NATIVE/${CMAKE_SYSTEM_NAME}/amd64/")
  endif()
else()
  message(WARNING "Unknown system type. Processor: ${CMAKE_SYSTEM_PROCESSOR}; System: ${CMAKE_SYSTEM_NAME}")
  set(JSG_BUNDLE "NATIVE/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}/")
endif()
message("-- [Java] Native libraries bundled into: ${JSG_BUNDLE}")

## Don't strip libraries if not in release mode
##
if(release)
  set(STRIP_COMMAND "${CMAKE_STRIP}")
else()
  set(STRIP_COMMAND "true")
endif()

## Here is how to build simgrid.jar
##
set(CMAKE_JAVA_TARGET_OUTPUT_NAME simgrid)
add_jar(SG_java_pre_jar ${JMSG_JAVA_SRC})

add_custom_command(
  COMMENT "Finalize simgrid.jar..."
  OUTPUT ${SIMGRID_JAR}_finalized
  DEPENDS simgrid SG_java SG_java_pre_jar
          ${SIMGRID_JAR} ${MANIFEST_FILE}
          ${CMAKE_BINARY_DIR}/lib/${LIBSIMGRID_SO}
          ${CMAKE_BINARY_DIR}/lib/${LIBSG_JAVA_SO}
          ${CMAKE_HOME_DIRECTORY}/COPYING
          ${CMAKE_HOME_DIRECTORY}/ChangeLog
          ${CMAKE_HOME_DIRECTORY}/ChangeLog.SimGrid-java
          ${CMAKE_HOME_DIRECTORY}/LICENSE-LGPL-2.1
  COMMAND ${CMAKE_COMMAND} -E remove_directory "NATIVE"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${JSG_BUNDLE}"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/lib/${LIBSIMGRID_SO}" "${JSG_BUNDLE}"
  COMMAND ${STRIP_COMMAND} -S "${JSG_BUNDLE}/${LIBSIMGRID_SO}"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/lib/${LIBSG_JAVA_SO}" "${JSG_BUNDLE}"
  COMMAND ${STRIP_COMMAND} -S "${JSG_BUNDLE}/${LIBSG_JAVA_SO}"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_HOME_DIRECTORY}/COPYING" "${JSG_BUNDLE}"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_HOME_DIRECTORY}/ChangeLog" "${JSG_BUNDLE}"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_HOME_DIRECTORY}/ChangeLog.SimGrid-java" "${JSG_BUNDLE}"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_HOME_DIRECTORY}/LICENSE-LGPL-2.1" "${JSG_BUNDLE}"
  COMMAND ${JAVA_ARCHIVE} -uvmf ${MANIFEST_FILE} ${SIMGRID_JAR} "NATIVE"
  COMMAND ${CMAKE_COMMAND} -E remove ${SIMGRID_JAR}_finalized
  COMMAND ${CMAKE_COMMAND} -E touch ${SIMGRID_JAR}_finalized
  )
add_custom_target(SG_java_jar ALL DEPENDS ${SIMGRID_JAR}_finalized)
