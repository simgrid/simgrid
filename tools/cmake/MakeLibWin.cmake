### Make Libs

add_library(simgrid SHARED ${simgrid_sources})

if(MSVC)
  set_target_properties(simgrid  PROPERTIES
       COMPILE_FLAGS "/DDLL_EXPORT"
       VERSION ${libsimgrid_version} )
else()
  set_target_properties(simgrid  PROPERTIES
       COMPILE_FLAGS "-DDLL_EXPORT"
       LINK_FLAGS "-shared"
       VERSION ${libsimgrid_version}
       PREFIX "lib" SUFFIX ".dll"
       IMPORT_PREFIX "lib" IMPORT_SUFFIX ".dll")

  set(SIMGRID_DEP "-lm")

  if (HAVE_BOOST_CONTEXTS)
    target_link_libraries(simgrid ${Boost_CONTEXT_LIBRARY})
  endif()

  if (HAVE_BOOST_STACKTRACE_BACKTRACE)
    target_link_libraries(simgrid ${Boost_STACKTRACE_BACKTRACE_LIBRARY})
  endif()

  if (HAVE_BOOST_ADDR2LINE_BACKTRACE)
    target_link_libraries(simgrid ${Boost_STACKTRACE_ADDR2LINE_LIBRARY})
  endif()
endif()

target_link_libraries(simgrid 	${SIMGRID_DEP})

# The library can obviously use the internal headers
set_property(TARGET simgrid
             APPEND PROPERTY INCLUDE_DIRECTORIES "${INTERNAL_INCLUDES}")
