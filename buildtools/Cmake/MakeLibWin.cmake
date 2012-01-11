### Make Libs
#>gcc c:\simgrid-trunk\examples\msg\icomms\peer.c -static -Lc:\simgrid-trunk\lib -lsimgrid -Ic:\simgrid-trunk\include -lwsock32	
add_library(simgrid STATIC ${simgrid_sources})
add_library(gras STATIC ${gras_sources})

set_target_properties(gras     PROPERTIES COMPILE_FLAGS "-D_XBT_DLL_STATIC"
                                                     OUTPUT_NAME   "gras")
set_target_properties(simgrid  PROPERTIES COMPILE_FLAGS "-D_XBT_DLL_STATIC"                                                  OUTPUT_NAME   "simgrid")

set(GRAS_DEP "ws2_32 -lpthread")
set(SIMGRID_DEP "ws2_32 -lpcre -lpthread")

if(ARCH_32_BITS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -march=i486")
else(ARCH_32_BITS)
        message(FATAL_ERROR "Sorry, Simgrid fails with full 64bits for now! Please contact us.")
endif(ARCH_32_BITS)

target_link_libraries(simgrid 	${SIMGRID_DEP})
target_link_libraries(gras 	${GRAS_DEP})