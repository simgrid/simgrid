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

if(enable_java)
  include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/MakeJava.cmake)
endif()

add_dependencies(simgrid maintainer_files)

# if supernovaeing, we need some depends to make sure that the source gets generated
if (enable_supernovae)
  add_dependencies(simgrid ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_sg.c)
  if(enable_lib_static)
    add_dependencies(simgrid_static ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_sg.c)
  endif()

  if(enable_smpi)
    add_dependencies(simgrid ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_smpi.c)
    if(enable_lib_static)
      add_dependencies(simgrid_static ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_smpi.c)
    endif()
  endif()
endif()

# Compute the dependencies of SimGrid
#####################################
set(SIMGRID_DEP "-lm")

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
  
  SET(SIMGRID_DEP "${SIMGRID_DEP} -lunwind")
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

# Compute the dependencies of SMPI
##################################
if(enable_smpi AND APPLE)
  set(SIMGRID_DEP "${SIMGRID_DEP} -Wl,-U -Wl,_smpi_simulated_main")
endif()

target_link_libraries(simgrid 	${SIMGRID_DEP})

# Pass dependencies to static libs
##################################
if(enable_lib_static)
  target_link_libraries(simgrid_static 	${SIMGRID_DEP})
  add_dependencies(simgrid_static maintainer_files)
  set_target_properties(simgrid_static PROPERTIES OUTPUT_NAME simgrid)
endif()

# Dependencies from maintainer mode
###################################
if(enable_maintainer_mode AND BISON_EXE AND LEX_EXE)
  add_dependencies(simgrid automaton_generated_src)
endif()
