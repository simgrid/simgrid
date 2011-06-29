cmake_minimum_required(VERSION 2.8)
### Need to set rc ccompiler before enable language
if(WIN32)
    SET(CMAKE_RC_COMPILER "windres")
endif(WIN32)

project(HelloWorld C)
set(CMAKE_C_FLAGS "" CACHE TYPE INTERNAL FORCE)
set(CMAKE_EXE_LINKER_FLAGS "" CACHE TYPE INTERNAL FORCE)

########
## GCC #
########
exec_program("${CMAKE_C_COMPILER} --version" OUTPUT_VARIABLE "COMPILER_C_VERSION")
string(REGEX MATCH "[0-9].[0-9].[0-9]" COMPILER_C_VERSION "${COMPILER_C_VERSION}")
string(REGEX MATCH "^[0-9].[0-9]" COMPILER_C_VERSION_MAJOR_MINOR "${COMPILER_C_VERSION}")
string(REPLACE "${COMPILER_C_VERSION_MAJOR_MINOR}." "" COMPILER_C_VERSION_PATCH "${COMPILER_C_VERSION}")

###############################
# Test the build dependencies #
###############################
include(FindSimGrid.cmake)

#########
# FLAGS #
#########
set(INCLUDE_PATH "-I$ENV{SIMGRID_ROOT}/include -I$ENV{SIMGRID_ROOT}/src -I$ENV{SIMGRID_ROOT}/src/include")
set(LIB_PATH "-L$ENV{SIMGRID_ROOT}/lib")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${INCLUDE_PATH} ${LIB_PATH}")

###########
# TARGETS #
###########

add_executable(HelloWorld HelloWorld)
target_link_libraries(HelloWorld simgrid)