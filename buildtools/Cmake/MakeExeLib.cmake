### Make Libs

if(enable_supernovae)
	include(${PROJECT_DIRECTORY}/buildtools/Cmake/Supernovae.cmake)
else(enable_supernovae)	
	add_library(simgrid SHARED ${simgrid_sources})
	add_library(simgrid_static STATIC ${simgrid_sources})
	add_library(gras SHARED ${gras_sources})
	if(enable_smpi)
		add_library(smpi SHARED ${SMPI_SRC})
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

if(HAVE_RUBY)
	set(SIMGRID_DEP "${SIMGRID_DEP} -l${RUBY_LIBRARY_NAME} -module")
	ADD_CUSTOM_COMMAND(
	  OUTPUT ${PROJECT_DIRECTORY}/src/bindings/ruby/libsimgrid.so
	  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_BINARY_DIR}/lib/libsimgrid.so ${PROJECT_DIRECTORY}/src/bindings/ruby/libsimgrid.so
	  COMMENT "Generating libsimgrid.so link for binding ruby..."
	)
	ADD_CUSTOM_TARGET(link_simgrid_ruby ALL
	                  DEPENDS ${PROJECT_DIRECTORY}/src/bindings/ruby/libsimgrid.so)       
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
    ADD_CUSTOM_COMMAND(
	  OUTPUT ${PROJECT_DIRECTORY}/examples/lua/simgrid.so
	  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_BINARY_DIR}/lib/libsimgrid.so ${PROJECT_DIRECTORY}/examples/lua/simgrid.so
	  COMMENT "Generating libsimgrid.so link for binding lua..."
	)
	ADD_CUSTOM_TARGET(link_simgrid_lua ALL
	                  DEPENDS ${PROJECT_DIRECTORY}/examples/lua/simgrid.so)       
endif(HAVE_LUA)

if(HAVE_GTNETS)
	SET(SIMGRID_DEP "${SIMGRID_DEP} -lgtnets")
endif(HAVE_GTNETS)

if(HAVE_POSIX_GETTIME)
	SET(SIMGRID_DEP "${SIMGRID_DEP} -lrt")
	SET(GRAS_DEP "${GRAS_DEP} -lrt")
endif(HAVE_POSIX_GETTIME)

target_link_libraries(simgrid 	${SIMGRID_DEP})
target_link_libraries(simgrid_static	${SIMGRID_DEP})
target_link_libraries(gras 	${GRAS_DEP})

add_dependencies(gras maintainer_files)
add_dependencies(simgrid maintainer_files)
add_dependencies(simgrid_static maintainer_files)
				
if(enable_smpi)
	target_link_libraries(smpi 	simgrid ${SMPI_DEP})
endif(enable_smpi)

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
add_subdirectory(${PROJECT_DIRECTORY}/examples/simdag/metaxml)
add_subdirectory(${PROJECT_DIRECTORY}/examples/simdag/properties)
add_subdirectory(${PROJECT_DIRECTORY}/examples/simdag/scheduling)
if(enable_smpi)
	add_subdirectory(${PROJECT_DIRECTORY}/examples/smpi)
endif(enable_smpi)