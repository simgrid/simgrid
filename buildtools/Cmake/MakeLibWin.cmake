### Make Libs
#>gcc c:\simgrid-trunk\examples\msg\icomms\peer.c -static -Lc:\simgrid-trunk\lib -lsimgrid -Ic:\simgrid-trunk\include -lwsock32	
add_library(simgrid STATIC ${simgrid_sources})
add_library(gras STATIC ${gras_sources})

if(MSVC)
    set_target_properties(gras     PROPERTIES COMPILE_FLAGS "/D _XBT_DLL_STATIC"
                                                     OUTPUT_NAME   "gras")
    set_target_properties(simgrid  PROPERTIES COMPILE_FLAGS "/D _XBT_DLL_STATIC"
                                                     OUTPUT_NAME   "simgrid")
else(MSVC)
    if(CMAKE_COMPILER_IS_GNUCC)
        set_target_properties(gras     PROPERTIES COMPILE_FLAGS "-D_XBT_DLL_STATIC"
                                                         OUTPUT_NAME   "gras")
        set_target_properties(simgrid  PROPERTIES COMPILE_FLAGS "-D_XBT_DLL_STATIC"
                                                         OUTPUT_NAME   "simgrid")
    else(CMAKE_COMPILER_IS_GNUCC)
        message(FATAL_ERROR "Compilateur non connu!!!")
    endif(CMAKE_COMPILER_IS_GNUCC)
endif(MSVC)

set(GRAS_DEP "ws2_32")
set(SIMGRID_DEP "ws2_32 -lpcre")

target_link_libraries(simgrid 	${SIMGRID_DEP})
target_link_libraries(gras 	${GRAS_DEP})
