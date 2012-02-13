### Make Libs
#>gcc c:\simgrid-trunk\examples\msg\icomms\peer.c -static -Lc:\simgrid-trunk\lib -lsimgrid -Ic:\simgrid-trunk\include -lwsock32	
add_library(simgrid STATIC ${simgrid_sources})
add_library(simgrid_shared SHARED ${simgrid_sources})
add_library(gras STATIC ${gras_sources})

set_target_properties(gras            PROPERTIES COMPILE_FLAGS "-D_XBT_DLL_STATIC -DDLL_STATIC" VERSION ${libgras_version}    OUTPUT_NAME "gras")
set_target_properties(simgrid         PROPERTIES COMPILE_FLAGS "-D_XBT_DLL_STATIC -DDLL_STATIC" VERSION ${libsimgrid_version} OUTPUT_NAME "simgrid")
set_target_properties(simgrid_shared  PROPERTIES COMPILE_FLAGS "-D_XBT_DLL_EXPORT -DDLL_EXPORT" VERSION ${libsimgrid_version} OUTPUT_NAME "simgrid_shared")

set(GRAS_DEP "ws2_32 -lpthread")
set(SIMGRID_DEP "ws2_32 -lpcre -lpthread")
set(SIMGRID_SHARED_DEP "ws2_32 -lpthread")

if(ARCH_32_BITS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -march=i486")
else(ARCH_32_BITS)
        message(FATAL_ERROR "Sorry, Simgrid fails with full 64bits for now! Please contact us.")
endif(ARCH_32_BITS)

target_link_libraries(gras 	${GRAS_DEP})
target_link_libraries(simgrid 	${SIMGRID_DEP})
target_link_libraries(simgrid_shared ${SIMGRID_SHARED_DEP} ${PATH_PCRE_LIB})

find_path(PEXPORTS_PATH NAMES pexports.exe PATHS NO_DEFAULT_PATHS)
message(STATUS "pexports: ${PEXPORTS_PATH}")
if(PEXPORTS_PATH)
add_custom_command(TARGET simgrid_shared POST_BUILD
COMMAND ${PEXPORTS_PATH}/pexports.exe ${CMAKE_BINARY_DIR}/lib/libsimgrid_shared.dll > ${CMAKE_BINARY_DIR}/lib/libsimgrid_shared.ref)
endif(PEXPORTS_PATH)