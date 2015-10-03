### Make Libs
#>gcc c:\simgrid-trunk\examples\msg\icomms\peer.c -static -Lc:\simgrid-trunk\lib -lsimgrid -Ic:\simgrid-trunk\include -lwsock32

add_library(simgrid SHARED ${simgrid_sources})

if(MSVC)
  set_target_properties(simgrid  PROPERTIES 
       COMPILE_FLAGS "/D_XBT_DLL_EXPORT /DDLL_EXPORT" 
       VERSION ${libsimgrid_version} )
else()
  set_target_properties(simgrid  PROPERTIES 
       COMPILE_FLAGS "-D_XBT_DLL_EXPORT -DDLL_EXPORT" 
       LINK_FLAGS "-shared" 
       VERSION ${libsimgrid_version} 
       PREFIX "lib" SUFFIX ".dll" 
       IMPORT_PREFIX "lib" IMPORT_SUFFIX ".dll")

  set(SIMGRID_DEP "-lws2_32 -lm")

  if (HAVE_PTHREAD)
    set(SIMGRID_DEP "${SIMGRID_DEP} -lpthread")
  endif()

  if(ARCH_32_BITS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -march=i486 -D_I_X86_")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -march=i486 -D_I_X86_")
  else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64 -D_AMD64_")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64 -D_AMD64_")
    #        message(FATAL_ERROR "Sorry, Simgrid fails with full 64bits for now! Please contact us.")
  endif()
endif()

target_link_libraries(simgrid 	${SIMGRID_DEP})

