### Make Libs

# On macOS, specify that rpath is useful to look for the dependencies
# See https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/RPATH-handling and Java.cmake
set(CMAKE_MACOSX_RPATH TRUE)
if(APPLE)
  SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE) # When installed, use system path
  set(CMAKE_SKIP_BUILD_RPATH FALSE)         # When executing from build tree, take the lib from the build path if exists
  set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) # When executing from build tree, take the lib from the system path if exists

  # add the current location of libsimgrid-java.dynlib as a location for libsimgrid.dynlib
  # (useful when unpacking the native libraries from the jarfile)
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}")
endif()

###############################
# Declare the library content #
###############################

# Actually declare our libraries
add_library(simgrid SHARED ${simgrid_sources})
set_target_properties(simgrid PROPERTIES VERSION ${libsimgrid_version})
# The library can obviously use the internal headers
set_property(TARGET simgrid
             APPEND PROPERTY INCLUDE_DIRECTORIES "${INTERNAL_INCLUDES}")

add_dependencies(simgrid maintainer_files)

if(enable_model-checking)
  add_executable(simgrid-mc ${MC_SIMGRID_MC_SRC})
  target_link_libraries(simgrid-mc simgrid)
  set_target_properties(simgrid-mc
                        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
  set_property(TARGET simgrid-mc
               APPEND PROPERTY INCLUDE_DIRECTORIES "${INTERNAL_INCLUDES}")
  install(TARGETS simgrid-mc # install that binary without breaking the rpath on Mac
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/)
  add_dependencies(tests-mc simgrid-mc)
endif()


# Compute the dependencies of SimGrid
#####################################
# search for dlopen
if("${CMAKE_SYSTEM_NAME}" MATCHES "kFreeBSD|Linux|SunOS")
  find_library(DL_LIBRARY dl)
endif()
mark_as_advanced(DL_LIBRARY)

if (HAVE_BOOST_CONTEXTS)
  target_link_libraries(simgrid ${Boost_CONTEXT_LIBRARY})
endif()

if (HAVE_BOOST_STACKTRACE_BACKTRACE)
  target_link_libraries(simgrid ${Boost_STACKTRACE_BACKTRACE_LIBRARY})
endif()

if (HAVE_BOOST_ADDR2LINE_BACKTRACE)
  target_link_libraries(simgrid ${Boost_STACKTRACE_ADDR2LINE_LIBRARY})
endif()

if(CMAKE_USE_PTHREADS_INIT)
  target_link_libraries(simgrid ${CMAKE_THREAD_LIBS_INIT})
endif()

if(HAVE_PAPI)
  SET(SIMGRID_DEP "${SIMGRID_DEP} -lpapi")
endif()

if(HAVE_GRAPHVIZ)
  if(HAVE_CGRAPH_LIB)
    SET(SIMGRID_DEP "${SIMGRID_DEP} -lcgraph")
  else()
    if(HAVE_AGRAPH_LIB)
      SET(SIMGRID_DEP "${SIMGRID_DEP} -lagraph -lcdt")
    endif()
  endif()
endif()

if(NOT ${DL_LIBRARY} STREQUAL "")
  SET(SIMGRID_DEP "${SIMGRID_DEP} ${DL_LIBRARY}")
endif()

if(HAVE_POSIX_GETTIME)
  SET(SIMGRID_DEP "${SIMGRID_DEP} -lrt")
endif()

if("${CMAKE_SYSTEM_NAME}" STREQUAL "FreeBSD")
  set(SIMGRID_DEP "${SIMGRID_DEP} -lprocstat")
endif()

# Compute the dependencies of SMPI
##################################

if(enable_smpi)
  add_executable(smpimain src/smpi/smpi_main.c)
  target_link_libraries(smpimain simgrid)
  set_target_properties(smpimain
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/simgrid)
  install(TARGETS smpimain # install that binary without breaking the rpath on Mac
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/simgrid)
  add_dependencies(tests smpimain)

  add_executable(smpireplaymain src/smpi/smpi_replay_main.cpp)
  target_compile_options(smpireplaymain PRIVATE -fpic)
  target_link_libraries(smpireplaymain simgrid -fpic -shared)
  set_target_properties(smpireplaymain
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/simgrid)
  install(TARGETS smpireplaymain # install that binary without breaking the rpath on Mac
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/simgrid)
  add_dependencies(tests smpireplaymain)

  if(SMPI_FORTRAN)
    if(CMAKE_Fortran_COMPILER_ID MATCHES "GNU")
      SET(SIMGRID_DEP "${SIMGRID_DEP} -lgfortran")
    elseif(CMAKE_Fortran_COMPILER_ID MATCHES "Intel")
      SET(SIMGRID_DEP "${SIMGRID_DEP} -lifcore")
    elseif(CMAKE_Fortran_COMPILER_ID MATCHES "PGI|Flang")
      SET(SIMGRID_DEP "${SIMGRID_DEP} -lflang")
      if("${CMAKE_SYSTEM}" MATCHES "FreeBSD")
        set(SIMGRID_DEP "${SIMGRID_DEP} -lexecinfo")
	if ("${CMAKE_SYSTEM_VERSION}" STRGREATER_EQUAL "12")
            set(SIMGRID_DEP "${SIMGRID_DEP} -lpgmath")
        endif()
        if ("${CMAKE_SYSTEM_VERSION}" MATCHES "12\.1")
            set(SIMGRID_DEP "${SIMGRID_DEP} -lomp")
        endif()
      endif()
    endif()
  endif()

endif()

if(enable_smpi AND APPLE)
  set(SIMGRID_DEP "${SIMGRID_DEP} -Wl,-U -Wl,_smpi_simulated_main")
endif()

# See https://github.com/HewlettPackard/foedus_code/blob/master/foedus-core/cmake/FindGccAtomic.cmake
FIND_LIBRARY(GCCLIBATOMIC_LIBRARY NAMES atomic atomic.so.1 libatomic.so.1
  HINTS
    $ENV{HOME}/local/lib64
    $ENV{HOME}/local/lib
    /usr/local/lib64
    /usr/local/lib
    /opt/local/lib64
    /opt/local/lib
    /usr/lib64
    /usr/lib
    /lib64
    /lib
)

# Fix a FTBFS on armel, mips, mipsel and friends (Debian's #872881)
if(CMAKE_COMPILER_IS_GNUCC AND GCCLIBATOMIC_LIBRARY)
    set(SIMGRID_DEP   "${SIMGRID_DEP}   -Wl,--as-needed -latomic -Wl,--no-as-needed")
endif()
mark_as_advanced(GCCLIBATOMIC_LIBRARY)

if(enable_model-checking AND (NOT LINKER_VERSION VERSION_LESS "2.30"))
    set(SIMGRID_DEP   "${SIMGRID_DEP}   -Wl,-znorelro -Wl,-znoseparate-code")
endif()

target_link_libraries(simgrid 	${SIMGRID_DEP})

# Dependencies from maintainer mode
###################################
if(enable_maintainer_mode)
  add_dependencies(simgrid smpi_generated_headers_call_location_tracing)
endif()
if(enable_maintainer_mode AND BISON_EXE AND LEX_EXE)
  add_dependencies(simgrid automaton_generated_src)
endif()
