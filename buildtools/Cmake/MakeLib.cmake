### Make Libs

###############################
# Declare the library content #
###############################
# If we want supernovae, rewrite the libs' content to use it
include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/Supernovae.cmake)

# Actually declare our libraries

add_library(simgrid SHARED ${simgrid_sources})
set_target_properties(simgrid PROPERTIES VERSION ${libsimgrid_version})

if(enable_lib_static)
  add_library(simgrid_static STATIC ${simgrid_sources})
endif()

if(enable_smpi)
  add_library(smpi SHARED ${SMPI_SRC})
  set_target_properties(smpi PROPERTIES VERSION ${libsmpi_version})
  if(enable_lib_static)
    add_library(smpi_static STATIC ${SMPI_SRC})
  endif()
endif()

if(enable_java)
  add_library(SG_java SHARED ${JMSG_C_SRC})
  set_target_properties(SG_java PROPERTIES VERSION ${libSG_java_version})
  get_target_property(COMMON_INCLUDES SG_java INCLUDE_DIRECTORIES)
  set_target_properties(SG_java PROPERTIES
    INCLUDE_DIRECTORIES "${COMMON_INCLUDES};${JNI_INCLUDE_DIRS}")
  add_dependencies(SG_java simgrid)

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

  set(CMAKE_JAVA_TARGET_OUTPUT_NAME simgrid)
  add_jar(SG_java_jar ${JMSG_JAVA_SRC})

  set(SIMGRID_JAR "${CMAKE_BINARY_DIR}/simgrid.jar")
  set(MANIFEST_FILE "${CMAKE_HOME_DIRECTORY}/src/bindings/java/MANIFEST.MF")

  if(CMAKE_SYSTEM_PROCESSOR MATCHES ".86")
    if(${ARCH_32_BITS})
      set(JSG_BUNDLE "NATIVE/${CMAKE_SYSTEM_NAME}/x86/")
    else()
      set(JSG_BUNDLE "NATIVE/${CMAKE_SYSTEM_NAME}/amd64/")
    endif()
  else()
    error("Unknown system type. Processor: ${CMAKE_SYSTEM_PROCESSOR}; System: ${CMAKE_SYSTEM_NAME}")
  endif()
  message("Native libraries bundled into: ${JSG_BUNDLE}")

  set(LIBSIMGRID_SO
    ${CMAKE_SHARED_LIBRARY_PREFIX}simgrid${CMAKE_SHARED_LIBRARY_SUFFIX})
  set(LIBSG_JAVA_SO
    ${CMAKE_SHARED_LIBRARY_PREFIX}simgrid${CMAKE_SHARED_LIBRARY_SUFFIX})

  add_custom_command(
    COMMENT "Finalize simgrid.jar..."
    OUTPUT ${SIMGRID_JAR}_finalized
    DEPENDS simgrid SG_java ${SIMGRID_JAR} ${MANIFEST_FILE}
            ${CMAKE_HOME_DIRECTORY}/COPYING
            ${CMAKE_HOME_DIRECTORY}/ChangeLog
            ${CMAKE_HOME_DIRECTORY}/ChangeLog.SimGrid-java
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMAND ${CMAKE_COMMAND} -E remove_directory "NATIVE"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${JSG_BUNDLE}"
    COMMAND ${CMAKE_COMMAND} -E copy "./lib/${LIBSIMGRID_SO}" "${JSG_BUNDLE}"
    COMMAND ${CMAKE_STRIP} -S "${JSG_BUNDLE}/${LIBSIMGRID_SO}"
    COMMAND ${CMAKE_COMMAND} -E copy "./lib/${LIBSG_JAVA_SO}" "${JSG_BUNDLE}"
    COMMAND ${CMAKE_STRIP} -S "${JSG_BUNDLE}/${LIBSG_JAVA_SO}"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_HOME_DIRECTORY}/COPYING" "${JSG_BUNDLE}"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_HOME_DIRECTORY}/ChangeLog" "${JSG_BUNDLE}"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_HOME_DIRECTORY}/ChangeLog.SimGrid-java" "${JSG_BUNDLE}"
    COMMAND ${JAVA_ARCHIVE} -uvmf ${MANIFEST_FILE} ${SIMGRID_JAR} "NATIVE"
    COMMAND ${CMAKE_COMMAND} -E touch ${SIMGRID_JAR}_finalized
    )
  add_custom_target(SG_java_jar_finalize DEPENDS ${SIMGRID_JAR}_finalized)
  add_dependencies(SG_java_jar SG_java_jar_finalize)
endif()

add_dependencies(simgrid maintainer_files)

# if supernovaeing, we need some depends to make sure that the source gets generated
if (enable_supernovae)
  add_dependencies(simgrid ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_sg.c)
  if(enable_lib_static)
    add_dependencies(simgrid_static ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_sg.c)
  endif()

  if(enable_smpi)
    add_dependencies(smpi ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_smpi.c)
    if(enable_lib_static)
      add_dependencies(smpi_static ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_smpi.c)
    endif()
  endif()
endif()

# Compute the dependencies of SimGrid
#####################################
set(SIMGRID_DEP "-lm -lpcre")

if(pthread)
  if(${CONTEXT_THREADS})
    SET(SIMGRID_DEP "${SIMGRID_DEP} -pthread")
  endif()
endif()

if(HAVE_LUA)
  ADD_CUSTOM_TARGET(link_simgrid_lua ALL
    DEPENDS 	simgrid
    ${CMAKE_BINARY_DIR}/examples/lua/simgrid.${LIB_EXE}
    ${CMAKE_BINARY_DIR}/examples/msg/masterslave/simgrid.${LIB_EXE}
    ${CMAKE_BINARY_DIR}/examples/simdag/simgrid.${LIB_EXE}
    )
  add_custom_command(
    OUTPUT 	${CMAKE_BINARY_DIR}/examples/lua/simgrid.${LIB_EXE}
    ${CMAKE_BINARY_DIR}/examples/msg/masterslave/simgrid.${LIB_EXE}
    ${CMAKE_BINARY_DIR}/examples/simdag/simgrid.${LIB_EXE}
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/examples/lua/simgrid.${LIB_EXE} # if it exists, creating the link fails. So cleanup before hand
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/examples/lua/
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_BINARY_DIR}/lib/libsimgrid.${LIB_EXE} ${CMAKE_BINARY_DIR}/examples/lua/simgrid.${LIB_EXE} #for test

    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/examples/msg/masterslave/simgrid.${LIB_EXE} # if it exists, creating the link fails. So cleanup before hand
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/examples/msg/masterslave/
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_BINARY_DIR}/lib/libsimgrid.${LIB_EXE} ${CMAKE_BINARY_DIR}/examples/msg/masterslave/simgrid.${LIB_EXE} #for test

    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/examples/simdag/simgrid.${LIB_EXE} # if it exists, creating the link fails. So cleanup before hand
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/examples/simdag/
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_BINARY_DIR}/lib/libsimgrid.${LIB_EXE} ${CMAKE_BINARY_DIR}/examples/simdag/simgrid.${LIB_EXE} #for test
    )
  SET(SIMGRID_DEP "${SIMGRID_DEP} -l${LIB_LUA_NAME}")
endif()

if(HAVE_GRAPHVIZ)
  if(HAVE_CGRAPH_LIB)
    SET(SIMGRID_DEP "${SIMGRID_DEP} -lcgraph")
  else()
    if(HAVE_AGRAPH_LIB)
      SET(SIMGRID_DEP "${SIMGRID_DEP} -lagraph -lcdt")
    endif()
  endif()
endif()

if(HAVE_GTNETS)
  SET(SIMGRID_DEP "${SIMGRID_DEP} -lgtnets")
endif()

if(HAVE_MC)
  # The availability of libunwind was checked in CompleteInFiles.cmake
  #   (that includes FindLibunwind.cmake), so simply load it now.
  
  # This supposes that the host machine is either an AMD or a X86.
  # This is deeply wrong, and should be fixed by manually loading -lunwind-PLAT (FIXME)
  if(PROCESSOR_x86_64)
    SET(SIMGRID_DEP "${SIMGRID_DEP} -lunwind-x86_64")
  else()    
    SET(SIMGRID_DEP "${SIMGRID_DEP} -lunwind-x86")
  endif()
endif()

if(MMALLOC_WANT_OVERRIDE_LEGACY AND HAVE_GNU_LD)
  SET(SIMGRID_DEP "${SIMGRID_DEP} -ldl")
endif()

if(HAVE_NS3)
  if(${NS3_VERSION} EQUAL 310)
    SET(SIMGRID_DEP "${SIMGRID_DEP} -lns3")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_NS3_3_10")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_NS3_3_10")
  else()
    SET(SIMGRID_DEP "${SIMGRID_DEP} -lns3-core -lns3-csma -lns3-point-to-point -lns3-internet -lns3-applications")
  endif()
endif()

if(HAVE_POSIX_GETTIME)
  SET(SIMGRID_DEP "${SIMGRID_DEP} -lrt")
endif()

target_link_libraries(simgrid 	${SIMGRID_DEP})

# Compute the dependencies of SMPI
##################################
set(SMPI_DEP "")
if(APPLE)
  set(SMPI_DEP "-Wl,-U -Wl,_smpi_simulated_main")
endif()
if(enable_smpi)
  target_link_libraries(smpi 	simgrid ${SMPI_DEP})
endif()

# Pass dependencies to static libs
##################################
if(enable_lib_static)
  target_link_libraries(simgrid_static 	${SIMGRID_DEP})
  add_dependencies(simgrid_static maintainer_files)
  set_target_properties(simgrid_static PROPERTIES OUTPUT_NAME simgrid)
  if(enable_smpi)
    target_link_libraries(smpi_static 	simgrid ${SMPI_DEP})
    set_target_properties(smpi_static PROPERTIES OUTPUT_NAME smpi)
  endif()
endif()

# Dependencies from maintainer mode
###################################
if(enable_maintainer_mode AND BISON_EXE AND LEX_EXE)
  add_dependencies(simgrid automaton_generated_src)
endif()
