cmake_minimum_required(VERSION 2.8.6)

include(UseJava)

# Rules to build libSG_java
#
add_library(SG_java SHARED ${JMSG_C_SRC})
set_target_properties(SG_java PROPERTIES VERSION ${libSG_java_version})
get_target_property(COMMON_INCLUDES SG_java INCLUDE_DIRECTORIES)
set_target_properties(SG_java PROPERTIES
  INCLUDE_DIRECTORIES "${COMMON_INCLUDES};${JNI_INCLUDE_DIRS}")
add_dependencies(SG_java simgrid)

get_target_property(CHECK_INCLUDES SG_java INCLUDE_DIRECTORIES)
message("SG_java includes = ${CHECK_INCLUDES}")

if(WIN32)
  get_target_property(SIMGRID_LIB_NAME_NAME SG_java LIBRARY_OUTPUT_NAME)
  set_target_properties(SG_java PROPERTIES
    LINK_FLAGS "-Wl,--subsystem,windows,--kill-at ${SIMGRID_LIB_NAME}"
    PREFIX "")
  find_path(PEXPORTS_PATH NAMES pexports.exe PATHS NO_DEFAULT_PATHS)
  message(STATUS "pexports: ${PEXPORTS_PATH}")
  if(PEXPORTS_PATH)
    add_custom_command(TARGET SG_java POST_BUILD
      COMMAND ${PEXPORTS_PATH}/pexports.exe ${CMAKE_BINARY_DIR}/SG_java.dll > ${CMAKE_BINARY_DIR}/SG_java.def)
  endif(PEXPORTS_PATH)
else()
  target_link_libraries(SG_java simgrid)
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
  set(JSG_BUNDLE "NATIVE/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR/")
endif()
message("Native libraries bundled into: ${JSG_BUNDLE}")

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
  COMMAND ${CMAKE_COMMAND} -E remove_directory "NATIVE"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${JSG_BUNDLE}"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/lib/${LIBSIMGRID_SO}" "${JSG_BUNDLE}"
  COMMAND ${STRIP_COMMAND} -S "${JSG_BUNDLE}/${LIBSIMGRID_SO}"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/lib/${LIBSG_JAVA_SO}" "${JSG_BUNDLE}"
  COMMAND ${STRIP_COMMAND} -S "${JSG_BUNDLE}/${LIBSG_JAVA_SO}"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_HOME_DIRECTORY}/COPYING" "${JSG_BUNDLE}"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_HOME_DIRECTORY}/ChangeLog" "${JSG_BUNDLE}"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_HOME_DIRECTORY}/ChangeLog.SimGrid-java" "${JSG_BUNDLE}"
  COMMAND ${JAVA_ARCHIVE} -uvmf ${MANIFEST_FILE} ${SIMGRID_JAR} "NATIVE"
  COMMAND ${CMAKE_COMMAND} -E remove ${SIMGRID_JAR}_finalized
  COMMAND ${CMAKE_COMMAND} -E touch ${SIMGRID_JAR}_finalized
  )
add_custom_target(SG_java_jar ALL DEPENDS ${SIMGRID_JAR}_finalized)
