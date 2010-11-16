### Make Libs

if(enable_supernovae)
	include(${PROJECT_DIRECTORY}/buildtools/Cmake/Supernovae.cmake)
else(enable_supernovae)	
	add_library(simgrid SHARED ${simgrid_sources})
	add_library(gras SHARED ${gras_sources})
	if(enable_lib_static)
		add_library(simgrid_static STATIC ${simgrid_sources})	
	endif(enable_lib_static)
	if(enable_smpi)
		add_library(smpi SHARED ${SMPI_SRC})
		if(enable_lib_static)
			add_library(smpi_static STATIC ${SMPI_SRC})	
		endif(enable_lib_static)
	endif(enable_smpi)
endif(enable_supernovae)

set_target_properties(simgrid PROPERTIES VERSION ${libsimgrid_version})
set_target_properties(gras PROPERTIES VERSION ${libgras_version})
if(enable_smpi)
	set_target_properties(smpi PROPERTIES VERSION ${libsmpi_version})
endif(enable_smpi)

set(GRAS_DEP "-lm -lpthread")
set(SIMGRID_DEP "-lm")
set(SMPI_DEP "")

if(HAVE_PCRE_LIB)
       SET(SIMGRID_DEP "${SIMGRID_DEP} -lpcre")
endif(HAVE_PCRE_LIB)

if(HAVE_RUBY)
	set(SIMGRID_DEP "${SIMGRID_DEP} -l${RUBY_LIBRARY_NAME} -module")
	ADD_CUSTOM_TARGET(link_simgrid_ruby ALL
	  DEPENDS simgrid ${PROJECT_DIRECTORY}/src/bindings/ruby/libsimgrid.${LIB_EXE}
	  )
	add_custom_command(
		OUTPUT ${PROJECT_DIRECTORY}/src/bindings/ruby/libsimgrid.${LIB_EXE}
	        COMMAND ${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/src/bindings/ruby/libsimgrid.${LIB_EXE} # if it exists, creating the link fails. So cleanup before hand
		COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_BINARY_DIR}/lib/libsimgrid.${LIB_EXE} ${PROJECT_DIRECTORY}/src/bindings/ruby/libsimgrid.${LIB_EXE}
	)
endif(HAVE_RUBY)

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	add_definitions("-D_XOPEN_SOURCE")
endif(CMAKE_SYSTEM_NAME MATCHES "Darwin")

if(pthread)
	if(with_context MATCHES pthread)
		SET(SIMGRID_DEP "${SIMGRID_DEP} -lpthread")
	endif(with_context MATCHES pthread)
	
	if(with_context MATCHES windows)
		SET(GRAS_DEP "msg")
	endif(with_context MATCHES windows)
endif(pthread)

if(HAVE_LUA)
	SET(SIMGRID_DEP "${SIMGRID_DEP} -ldl -l${liblua}")   
	  
    ADD_CUSTOM_TARGET(link_simgrid_lua ALL
      DEPENDS 	simgrid
      			${PROJECT_DIRECTORY}/examples/lua/simgrid.${LIB_EXE}
				${PROJECT_DIRECTORY}/examples/msg/masterslave/simgrid.${LIB_EXE}
				${PROJECT_DIRECTORY}/examples/simdag/simgrid.${LIB_EXE}
	)
	add_custom_command(
		OUTPUT 	${PROJECT_DIRECTORY}/examples/lua/simgrid.${LIB_EXE}
				${PROJECT_DIRECTORY}/examples/msg/masterslave/simgrid.${LIB_EXE}
				${PROJECT_DIRECTORY}/examples/simdag/simgrid.${LIB_EXE}
		COMMAND ${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/examples/lua/simgrid.${LIB_EXE} # if it exists, creating the link fails. So cleanup before hand
		COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_BINARY_DIR}/lib/libsimgrid.${LIB_EXE} ${PROJECT_DIRECTORY}/examples/lua/simgrid.${LIB_EXE} #for test
		
		COMMAND ${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/examples/msg/masterslave/simgrid.${LIB_EXE} # if it exists, creating the link fails. So cleanup before hand
	  	COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_BINARY_DIR}/lib/libsimgrid.${LIB_EXE} ${PROJECT_DIRECTORY}/examples/msg/masterslave/simgrid.${LIB_EXE} #for test
		
		COMMAND ${CMAKE_COMMAND} -E remove -f ${PROJECT_DIRECTORY}/examples/simdag/simgrid.${LIB_EXE} # if it exists, creating the link fails. So cleanup before hand
	  	COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_BINARY_DIR}/lib/libsimgrid.${LIB_EXE} ${PROJECT_DIRECTORY}/examples/simdag/simgrid.${LIB_EXE} #for test			
	)
endif(HAVE_LUA)

if(HAVE_CGRAPH_LIB AND HAVE_CGRAPH_H)
	SET(SIMGRID_DEP "${SIMGRID_DEP} -lcgraph")
endif(HAVE_CGRAPH_LIB AND HAVE_CGRAPH_H)

if(HAVE_GTNETS)
	SET(SIMGRID_DEP "${SIMGRID_DEP} -lgtnets")
endif(HAVE_GTNETS)

if(HAVE_POSIX_GETTIME)
	SET(SIMGRID_DEP "${SIMGRID_DEP} -lrt")
	SET(GRAS_DEP "${GRAS_DEP} -lrt")
endif(HAVE_POSIX_GETTIME)

target_link_libraries(simgrid 	${SIMGRID_DEP})
target_link_libraries(gras 	${GRAS_DEP})
add_dependencies(gras maintainer_files)
add_dependencies(simgrid maintainer_files)				
if(enable_smpi)
	target_link_libraries(smpi 	simgrid ${SMPI_DEP})
endif(enable_smpi)

if(enable_lib_static)
	target_link_libraries(simgrid_static 	${SIMGRID_DEP})
	add_dependencies(simgrid_static maintainer_files)
	set_target_properties(simgrid_static PROPERTIES OUTPUT_NAME simgrid)
	if(enable_smpi)
		target_link_libraries(smpi_static 	simgrid ${SMPI_DEP})
		set_target_properties(smpi_static PROPERTIES OUTPUT_NAME smpi)
	endif(enable_smpi)
endif(enable_lib_static)

### Make EXEs

#src/testall
add_subdirectory(${PROJECT_DIRECTORY}/src)

#tools/gras
add_subdirectory(${PROJECT_DIRECTORY}/tools/gras)

#tools/tesh
add_subdirectory(${PROJECT_DIRECTORY}/tools/tesh)

#testsuite/xbt
add_subdirectory(${PROJECT_DIRECTORY}/testsuite/xbt)

#testsuite/surf
add_subdirectory(${PROJECT_DIRECTORY}/testsuite/surf)

#testsuite/simdag
add_subdirectory(${PROJECT_DIRECTORY}/testsuite/simdag)

#teshsuite
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

#examples
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
if(HAVE_CGRAPH_H)
add_subdirectory(${PROJECT_DIRECTORY}/examples/simdag/dot)
endif(HAVE_CGRAPH_H)
add_subdirectory(${PROJECT_DIRECTORY}/examples/simdag/metaxml)
add_subdirectory(${PROJECT_DIRECTORY}/examples/simdag/properties)
add_subdirectory(${PROJECT_DIRECTORY}/examples/simdag/scheduling)
if(enable_smpi)
	add_subdirectory(${PROJECT_DIRECTORY}/examples/smpi)
endif(enable_smpi)
