### Make Libs


# Try to make Mac a bit more complient to open source standards
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	add_definitions("-D_XOPEN_SOURCE")
endif(CMAKE_SYSTEM_NAME MATCHES "Darwin")

###############################
# Declare the library content #
###############################
# If we want supernovae, rewrite the libs' content to use it
include(${PROJECT_DIRECTORY}/buildtools/Cmake/Supernovae.cmake)

# Actually declare our libraries

add_library(simgrid SHARED ${simgrid_sources})
set_target_properties(simgrid PROPERTIES VERSION ${libsimgrid_version})

add_library(gras SHARED ${gras_sources})
set_target_properties(gras PROPERTIES VERSION ${libgras_version})

if(enable_lib_static)
	add_library(simgrid_static STATIC ${simgrid_sources})	
endif(enable_lib_static)

if(enable_smpi)
	add_library(smpi SHARED ${SMPI_SRC})
	set_target_properties(smpi PROPERTIES VERSION ${libsmpi_version})
	if(enable_lib_static)
		add_library(smpi_static STATIC ${SMPI_SRC})	
	endif(enable_lib_static)
endif(enable_smpi)

add_dependencies(gras maintainer_files)
add_dependencies(simgrid maintainer_files)				

# if supernovaeing, we need some depends to make sure that the source gets generated
if (enable_supernovae)
	add_dependencies(simgrid ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_sg.c)
	if(enable_lib_static)
		add_dependencies(simgrid_static ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_sg.c)
	endif(enable_lib_static)
	add_dependencies(gras ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_gras.c)

	if(enable_smpi)
		add_dependencies(smpi ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_smpi.c)
		if(enable_lib_static)
			add_dependencies(smpi_static ${CMAKE_CURRENT_BINARY_DIR}/src/supernovae_smpi.c)
		endif(enable_lib_static)
	endif(enable_smpi)
endif(enable_supernovae)	

# Compute the dependencies of GRAS
##################################
set(GRAS_DEP "-lm -lpthread")

if(HAVE_POSIX_GETTIME)
	SET(GRAS_DEP "${GRAS_DEP} -lrt")
endif(HAVE_POSIX_GETTIME)

# the following is probably unneed since it kills the previous
# GRAS_DEP (and is thus probably invalid).
# My guess is that pthread is never true [Mt]
# FIXME: KILLME if we get a working windows with that?
if(pthread AND (with_context MATCHES windows))
		SET(GRAS_DEP "msg")
endif(pthread AND (with_context MATCHES windows))

target_link_libraries(gras 	${GRAS_DEP})

# Compute the dependencies of SimGrid
#####################################
set(SIMGRID_DEP "-lm")
if(HAVE_PCRE_LIB)
       SET(SIMGRID_DEP "${SIMGRID_DEP} -lpcre")
endif(HAVE_PCRE_LIB)
if(HAVE_RUBY)
	set(SIMGRID_DEP "${SIMGRID_DEP} -l${RUBY_LIBRARY_NAME}")
endif(HAVE_RUBY)
if(pthread)
	if(with_context MATCHES pthread)
		SET(SIMGRID_DEP "${SIMGRID_DEP} -lpthread")
	endif(with_context MATCHES pthread)	
endif(pthread)

if(HAVE_LUA)
	SET(SIMGRID_DEP "${SIMGRID_DEP} -ldl -l${LIB_LUA_NAME}")   	  
endif(HAVE_LUA)

if(HAVE_GRAPHVIZ)
    if(HAVE_CGRAPH_LIB)
	    SET(SIMGRID_DEP "${SIMGRID_DEP} -lcgraph")
	else(HAVE_CGRAPH_LIB)
        if(HAVE_AGRAPH_LIB)
    	    SET(SIMGRID_DEP "${SIMGRID_DEP} -lagraph -lcdt")
        endif(HAVE_AGRAPH_LIB)	
    endif(HAVE_CGRAPH_LIB)    	    
endif(HAVE_GRAPHVIZ)

if(HAVE_GTNETS)
	SET(SIMGRID_DEP "${SIMGRID_DEP} -lgtnets")
endif(HAVE_GTNETS)

if(HAVE_POSIX_GETTIME)
	SET(SIMGRID_DEP "${SIMGRID_DEP} -lrt")
endif(HAVE_POSIX_GETTIME)

target_link_libraries(simgrid 	${SIMGRID_DEP})

# Compute the dependencies of SMPI
##################################
set(SMPI_LDEP "")
if(APPLE)
    set(SMPI_LDEP "-Wl,-U -Wl,_smpi_simulated_main")
endif(APPLE)
if(enable_smpi)
	target_link_libraries(smpi 	simgrid ${SMPI_LDEP})
endif(enable_smpi)

# Pass dependencies to static libs
##################################
if(enable_lib_static)
	target_link_libraries(simgrid_static 	${SIMGRID_DEP})
	add_dependencies(simgrid_static maintainer_files)
	set_target_properties(simgrid_static PROPERTIES OUTPUT_NAME simgrid)
	if(enable_smpi)
		target_link_libraries(smpi_static 	simgrid ${SMPI_LDEP})
		set_target_properties(smpi_static PROPERTIES OUTPUT_NAME smpi)
	endif(enable_smpi)
endif(enable_lib_static)

###################################################################
### Load all files declaring binaries (tools, examples and tests) #
###################################################################
add_subdirectory(${PROJECT_DIRECTORY}/tools/gras)
add_subdirectory(${PROJECT_DIRECTORY}/tools/tesh)

add_subdirectory(${PROJECT_DIRECTORY}/testsuite/xbt)
add_subdirectory(${PROJECT_DIRECTORY}/testsuite/surf)
add_subdirectory(${PROJECT_DIRECTORY}/testsuite/simdag)
add_subdirectory(${PROJECT_DIRECTORY}/teshsuite/xbt)
add_subdirectory(${PROJECT_DIRECTORY}/teshsuite/gras/datadesc)
add_subdirectory(${PROJECT_DIRECTORY}/teshsuite/gras/msg_handle)
add_subdirectory(${PROJECT_DIRECTORY}/teshsuite/gras/empty_main)
add_subdirectory(${PROJECT_DIRECTORY}/teshsuite/gras/small_sleep)
add_subdirectory(${PROJECT_DIRECTORY}/teshsuite/simdag)
add_subdirectory(${PROJECT_DIRECTORY}/teshsuite/simdag/network)
add_subdirectory(${PROJECT_DIRECTORY}/teshsuite/simdag/network/p2p)
add_subdirectory(${PROJECT_DIRECTORY}/teshsuite/simdag/network/mxn)
add_subdirectory(${PROJECT_DIRECTORY}/teshsuite/simdag/partask)
add_subdirectory(${PROJECT_DIRECTORY}/teshsuite/simdag/platforms)
add_subdirectory(${PROJECT_DIRECTORY}/teshsuite/msg)

add_subdirectory(${PROJECT_DIRECTORY}/examples/gras/ping)
add_subdirectory(${PROJECT_DIRECTORY}/examples/gras/rpc)
add_subdirectory(${PROJECT_DIRECTORY}/examples/gras/spawn)
add_subdirectory(${PROJECT_DIRECTORY}/examples/gras/timer)
add_subdirectory(${PROJECT_DIRECTORY}/examples/gras/chrono)
add_subdirectory(${PROJECT_DIRECTORY}/examples/gras/mutual_exclusion/simple_token)
add_subdirectory(${PROJECT_DIRECTORY}/examples/gras/mmrpc)
add_subdirectory(${PROJECT_DIRECTORY}/examples/gras/all2all)
add_subdirectory(${PROJECT_DIRECTORY}/examples/gras/pmm)
add_subdirectory(${PROJECT_DIRECTORY}/examples/gras/synchro)
add_subdirectory(${PROJECT_DIRECTORY}/examples/gras/properties)
add_subdirectory(${PROJECT_DIRECTORY}/examples/gras/console)

add_subdirectory(${PROJECT_DIRECTORY}/examples/msg/properties)
add_subdirectory(${PROJECT_DIRECTORY}/examples/msg/actions)
add_subdirectory(${PROJECT_DIRECTORY}/examples/msg/migration)
add_subdirectory(${PROJECT_DIRECTORY}/examples/msg/sendrecv)
add_subdirectory(${PROJECT_DIRECTORY}/examples/msg/suspend)
add_subdirectory(${PROJECT_DIRECTORY}/examples/msg/parallel_task)
add_subdirectory(${PROJECT_DIRECTORY}/examples/msg/priority)
add_subdirectory(${PROJECT_DIRECTORY}/examples/msg/masterslave)
add_subdirectory(${PROJECT_DIRECTORY}/examples/msg/trace)
add_subdirectory(${PROJECT_DIRECTORY}/examples/msg/tracing)
add_subdirectory(${PROJECT_DIRECTORY}/examples/msg/icomms)

if(HAVE_MC)
	add_subdirectory(${PROJECT_DIRECTORY}/examples/msg/mc)
endif(HAVE_MC)

if(HAVE_GTNETS)
	add_definitions("-lgtnets -L${gtnets_path}/lib -I${gtnets_path}/include/gtnets")
	add_subdirectory(${PROJECT_DIRECTORY}/examples/msg/gtnets)
endif(HAVE_GTNETS)

add_subdirectory(${PROJECT_DIRECTORY}/examples/amok/bandwidth)
add_subdirectory(${PROJECT_DIRECTORY}/examples/amok/saturate)

add_subdirectory(${PROJECT_DIRECTORY}/examples/simdag)
add_subdirectory(${PROJECT_DIRECTORY}/examples/simdag/dax)
if(HAVE_GRAPHVIZ)
  add_subdirectory(${PROJECT_DIRECTORY}/examples/simdag/dot)
endif(HAVE_GRAPHVIZ)
add_subdirectory(${PROJECT_DIRECTORY}/examples/simdag/metaxml)
add_subdirectory(${PROJECT_DIRECTORY}/examples/simdag/properties)
add_subdirectory(${PROJECT_DIRECTORY}/examples/simdag/scheduling)

if(enable_smpi)
	add_subdirectory(${PROJECT_DIRECTORY}/examples/smpi)
endif(enable_smpi)
