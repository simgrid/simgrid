### Make Libs
#>gcc c:\simgrid-trunk\examples\msg\icomms\peer.c -static -Lc:\simgrid-trunk\lib -lsimgrid -Ic:\simgrid-trunk\include -lwsock32
add_library(simgrid SHARED ${simgrid_sources})
add_library(gras SHARED ${gras_sources})

set_target_properties(gras     PROPERTIES COMPILE_FLAGS "-D_XBT_DLL_EXPORT -DDLL_EXPORT" VERSION ${libgras_version}    OUTPUT_NAME "gras")
set_target_properties(simgrid  PROPERTIES COMPILE_FLAGS "-D_XBT_DLL_EXPORT -DDLL_EXPORT" VERSION ${libsimgrid_version} OUTPUT_NAME "simgrid")

if(enable_smpi)
  add_library(smpi SHARED ${SMPI_SRC})
  set_target_properties(smpi PROPERTIES COMPILE_FLAGS "-D_XBT_DLL_EXPORT -DDLL_EXPORT" VERSION ${libsmpi_version} OUTPUT_NAME "smpi")
endif()

# libpthreadGC2.dll
if(ARCH_32_BITS)
  find_library(PATH_PTHREAD_LIB
    NAMES pthreadGC2.dll
    HINTS
    $ENV{PATH}
    PATH_SUFFIXES bin/ c/bin
    )
else()
  find_library(PATH_PTHREAD_LIB
    NAMES pthreadGC2-w64.dll
    HINTS
    $ENV{PATH}
    PATH_SUFFIXES bin/ c/bin
    )
endif()

set(GRAS_DEP "ws2_32 ${PATH_PTHREAD_LIB}")
set(SIMGRID_DEP "ws2_32 ${PATH_PCRE_LIB} ${PATH_PTHREAD_LIB}")
set(SMPI_DEP "${LIBRARY_OUTPUT_PATH}/libsimgrid.dll")

if(ARCH_32_BITS)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -march=i486")
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64")
  #        message(FATAL_ERROR "Sorry, Simgrid fails with full 64bits for now! Please contact us.")
endif()

target_link_libraries(gras 	${GRAS_DEP})
target_link_libraries(simgrid 	${SIMGRID_DEP})
if(enable_smpi)
add_dependencies(smpi simgrid)
target_link_libraries(smpi 	${SMPI_DEP})
endif()

find_path(PEXPORTS_PATH NAMES pexports.exe PATHS NO_DEFAULT_PATHS)
message(STATUS "pexports: ${PEXPORTS_PATH}")
if(PEXPORTS_PATH)
  add_custom_command(TARGET simgrid POST_BUILD
    COMMAND ${PEXPORTS_PATH}/pexports.exe ${CMAKE_BINARY_DIR}/lib/libsimgrid.dll > ${CMAKE_BINARY_DIR}/lib/libsimgrid.def)
endif()

if(enable_smpi)
  add_library(smpi SHARED ${SMPI_SRC})
  set_target_properties(smpi PROPERTIES VERSION ${libsmpi_version} OUTPUT_NAME "smpi")

  set(SMPI_LDEP "")
  target_link_libraries(smpi 	simgrid ${SMPI_LDEP})
endif()
