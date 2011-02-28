### Make Libs


# Try to make Mac a bit more complient to open source standards
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	add_definitions("-D_XOPEN_SOURCE")
endif(CMAKE_SYSTEM_NAME MATCHES "Darwin")

###############################
# Declare the library content #
###############################
# If we want supernovae, rewrite the libs' content to use it
include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/Supernovae.cmake)

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
if(with_context MATCHES windows)
if(pthread)
		SET(GRAS_DEP "msg")
endif(pthread)
endif(with_context MATCHES windows)
target_link_libraries(gras 	${GRAS_DEP})

# Compute the dependencies of SimGrid
#####################################
set(SIMGRID_DEP "-lm")
if(HAVE_PCRE_LIB)
       SET(SIMGRID_DEP "${SIMGRID_DEP} -lpcre")
endif(HAVE_PCRE_LIB)

if(pthread)
	if(${CONTEXT_THREADS})
		SET(SIMGRID_DEP "${SIMGRID_DEP} -lpthread")
	endif(${CONTEXT_THREADS})	
endif(pthread)

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