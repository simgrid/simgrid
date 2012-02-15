### This is a template for building targets with simgrid
cmake_minimum_required(VERSION 2.8)

### Need to set rc ccompiler before enable language
if(WIN32)
    SET(CMAKE_RC_COMPILER "windres")
    set(LIBRARIES_DEP simgrid pcre ws2_32 pthread)
else(WIN32)
    set(LIBRARIES_DEP simgrid pcre pthread)
endif(WIN32)

project(MY_SIMGRID_PROJECT C)

set(CMAKE_C_FLAGS "" CACHE TYPE INTERNAL FORCE)
set(CMAKE_EXE_LINKER_FLAGS "" CACHE TYPE INTERNAL FORCE)

###############################
# Test the build dependencies #
###############################
include(FindPCRE.cmake)
message(STATUS "Looking for lib Simgrid")
if("$ENV{SIMGRID_ROOT}" STREQUAL "")
    message(STATUS "Looking for lib Simgrid - Not found")
    message(FATAL_ERROR "Simgrid not found, reinstall it or set SIMGRID_ROOT")
else("$ENV{SIMGRID_ROOT}" STREQUAL "")
    link_directories($ENV{SIMGRID_ROOT}/lib)
    include_directories($ENV{SIMGRID_ROOT}/include)
    include_directories($ENV{SIMGRID_ROOT}/src)
    include_directories($ENV{SIMGRID_ROOT}/src/include)
    message(STATUS "Looking for lib Simgrid - found") 
endif("$ENV{SIMGRID_ROOT}" STREQUAL "")

################
# FIND TARGETS #
################
file(GLOB SOURCE_FILE
RELATIVE ${CMAKE_HOME_DIRECTORY}/
"*.c"
)
string(REPLACE ".c" "" TARGET_NAME ${SOURCE_FILE})

foreach(target "${TARGET_NAME}")
    add_executable(${target} "${target}.c")
    message(STATUS "source_file: ${target}.c")
    message(STATUS "target name: ${target}.exe")
    # Any targets need to be linked with libraries
    target_link_libraries(${target} ${LIBRARIES_DEP})
endforeach(target ${SOURCE_FILE})
