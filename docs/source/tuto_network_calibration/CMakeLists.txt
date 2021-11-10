cmake_minimum_required(VERSION 2.8.8)
project(tuto_network)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/" "../../../")
find_package(SimGrid REQUIRED)
include_directories(${SimGrid_INCLUDE_DIR})

set(SIMULATOR_DHIST_SOURCES dahu_platform_dhist.cpp Utils.cpp)
add_library(dahu_dhist SHARED ${SIMULATOR_DHIST_SOURCES})
target_link_libraries(dahu_dhist ${SimGrid_LIBRARY})
set_target_properties(dahu_dhist
    PROPERTIES SUFFIX .so
)

set(SIMULATOR_CKMEANS_SOURCES dahu_platform_ckmeans.cpp Utils.cpp)
add_library(dahu_ckmeans SHARED ${SIMULATOR_CKMEANS_SOURCES})
target_link_libraries(dahu_ckmeans ${SimGrid_LIBRARY})
set_target_properties(dahu_ckmeans
    PROPERTIES SUFFIX .so
)
