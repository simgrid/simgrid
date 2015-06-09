### Make Libs
#>gcc c:\simgrid-trunk\examples\msg\icomms\peer.c -static -Lc:\simgrid-trunk\lib -lsimgrid -Ic:\simgrid-trunk\include -lwsock32

if(enable_java)
  include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/MakeJava.cmake)
endif()

add_library(simgrid SHARED ${simgrid_sources})

set_target_properties(simgrid  PROPERTIES COMPILE_FLAGS "-D_XBT_DLL_EXPORT -DDLL_EXPORT" LINK_FLAGS "-shared" VERSION ${libsimgrid_version} PREFIX "lib" SUFFIX ".dll" IMPORT_PREFIX "lib" IMPORT_SUFFIX ".dll")

set(SIMGRID_DEP "-lws2_32 -lpthread -lm")

if(ARCH_32_BITS)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -march=i486 -D_I_X86_")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -march=i486 -D_I_X86_")
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64 -D_AMD64_")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64 -D_AMD64_")
  #        message(FATAL_ERROR "Sorry, Simgrid fails with full 64bits for now! Please contact us.")
endif()

target_link_libraries(simgrid 	${SIMGRID_DEP})

find_path(PEXPORTS_PATH NAMES pexports.exe PATHS NO_DEFAULT_PATHS)
message(STATUS "pexports: ${PEXPORTS_PATH}")
if(PEXPORTS_PATH)
  add_custom_command(TARGET simgrid POST_BUILD
    COMMAND ${PEXPORTS_PATH}/pexports.exe ${CMAKE_BINARY_DIR}/lib/libsimgrid.dll > ${CMAKE_BINARY_DIR}/lib/libsimgrid.def)
endif()
