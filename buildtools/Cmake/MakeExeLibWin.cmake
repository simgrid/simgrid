### Make Libs
	
add_library(simgrid SHARED ${simgrid_sources})
add_library(simgrid_static STATIC ${simgrid_sources})
add_library(gras SHARED ${gras_sources})
add_library(gras_static STATIC ${gras_sources})

set_target_properties(simgrid PROPERTIES VERSION ${libsimgrid_version})
set_target_properties(gras PROPERTIES VERSION ${libgras_version})


if(MSVC)
    set_target_properties(gras            PROPERTIES COMPILE_FLAGS "/D _XBT_DLL_EXPORT")
    set_target_properties(gras_static     PROPERTIES COMPILE_FLAGS "/D _XBT_DLL_STATIC"
                                                     OUTPUT_NAME   "gras")
    set_target_properties(simgrid         PROPERTIES COMPILE_FLAGS "/D _XBT_DLL_EXPORT")
    set_target_properties(simgrid_static  PROPERTIES COMPILE_FLAGS "/D _XBT_DLL_STATIC"
                                                     OUTPUT_NAME   "simgrid")
else(MSVC)
    if(CMAKE_COMPILER_IS_GNUCC)
        set_target_properties(gras            PROPERTIES COMPILE_FLAGS "-D _XBT_DLL_EXPORT")
        set_target_properties(gras_static     PROPERTIES COMPILE_FLAGS "-D _XBT_DLL_STATIC"
                                                         OUTPUT_NAME   "gras")
        set_target_properties(simgrid         PROPERTIES COMPILE_FLAGS "-D _XBT_DLL_EXPORT")
        set_target_properties(simgrid_static  PROPERTIES COMPILE_FLAGS "-D _XBT_DLL_STATIC"
                                                         OUTPUT_NAME   "simgrid")
    else(CMAKE_COMPILER_IS_GNUCC)
        message(FATAL_ERROR "Compilateur non connu!!!")
    endif(CMAKE_COMPILER_IS_GNUCC)
endif(MSVC)

set(GRAS_DEP "wsock32")
set(SIMGRID_DEP "wsock32")

target_link_libraries(simgrid 	${SIMGRID_DEP})
target_link_libraries(simgrid_static	${SIMGRID_DEP})
target_link_libraries(gras 	${GRAS_DEP})
target_link_libraries(gras_static 	${GRAS_DEP})

### Make EXEs

#src/testall
add_subdirectory(${PROJECT_DIRECTORY}/src)

#examples
add_subdirectory(${PROJECT_DIRECTORY}/examples/msg/icomms)

#testsuite/xbt
add_subdirectory(${PROJECT_DIRECTORY}/testsuite/xbt)

#testsuite/surf
add_subdirectory(${PROJECT_DIRECTORY}/testsuite/surf)

#testsuite/simdag
add_subdirectory(${PROJECT_DIRECTORY}/testsuite/simdag)