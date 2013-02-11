### Make Libs
#>gcc c:\simgrid-trunk\examples\msg\icomms\peer.c -static -Lc:\simgrid-trunk\lib -lsimgrid -Ic:\simgrid-trunk\include -lwsock32
add_library(simgrid SHARED ${simgrid_sources})

set_target_properties(simgrid  PROPERTIES COMPILE_FLAGS "-D_XBT_DLL_EXPORT -DDLL_EXPORT" LINK_FLAGS "-shared" VERSION ${libsimgrid_version} OUTPUT_NAME "simgrid" IMPORT_PREFIX "lib" IMPORT_SUFFIX ".dll")


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

set(SIMGRID_DEP "-lws2_32 -L${PATH_PCRE_LIB} -L${PATH_PTHREAD_LIB} -lm -lpcre -lpthreadGC2")

#set(SMPI_DEP "${LIBRARY_OUTPUT_PATH}/libsimgrid.dll")

if(ARCH_32_BITS)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -march=i486")
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64")
  #        message(FATAL_ERROR "Sorry, Simgrid fails with full 64bits for now! Please contact us.")
endif()

target_link_libraries(simgrid 	${SIMGRID_DEP})

find_path(PEXPORTS_PATH NAMES pexports.exe PATHS NO_DEFAULT_PATHS)
message(STATUS "pexports: ${PEXPORTS_PATH}")
if(PEXPORTS_PATH)
  add_custom_command(TARGET simgrid POST_BUILD
    COMMAND ${PEXPORTS_PATH}/pexports.exe ${CMAKE_BINARY_DIR}/lib/simgrid.dll > ${CMAKE_BINARY_DIR}/lib/simgrid.def)
endif()

if(enable_smpi)
  add_library(smpi SHARED ${SMPI_SRC})
  set_target_properties(smpi PROPERTIES COMPILE_FLAGS "-D_XBT_DLL_EXPORT -DDLL_EXPORT" LINK_FLAGS "-shared" VERSION ${libsmpi_version} OUTPUT_NAME "smpi" IMPORT_SUFFIX ".dll")
  target_link_libraries(smpi simgrid)
endif()
