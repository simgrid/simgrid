### Make Libs
#>gcc c:\simgrid-trunk\examples\msg\icomms\peer.c -static -Lc:\simgrid-trunk\lib -lsimgrid -Ic:\simgrid-trunk\include -lwsock32	
add_library(simgrid STATIC ${simgrid_sources})
add_library(gras STATIC ${gras_sources})

if(CMAKE_COMPILER_IS_GNUCC)
    set_target_properties(gras     PROPERTIES COMPILE_FLAGS "-D_XBT_DLL_STATIC"
                                                     OUTPUT_NAME   "gras")
    set_target_properties(simgrid  PROPERTIES COMPILE_FLAGS "-D_XBT_DLL_STATIC"
                                                     OUTPUT_NAME   "simgrid")
endif(CMAKE_COMPILER_IS_GNUCC)

set(GRAS_DEP "ws2_32 -lpthread")
set(SIMGRID_DEP "ws2_32 -lpcre -lpthread")

target_link_libraries(simgrid 	${SIMGRID_DEP})
target_link_libraries(gras 	${GRAS_DEP})
