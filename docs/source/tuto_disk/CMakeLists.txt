cmake_minimum_required(VERSION 2.8.8)
project(tuto_disk)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/" "../../../")
find_package(SimGrid REQUIRED)
include_directories(${SimGrid_INCLUDE_DIR})

set(SIMULATOR_SOURCES tuto_disk.cpp)
add_executable(tuto_disk ${SIMULATOR_SOURCES})
target_link_libraries(tuto_disk ${SimGrid_LIBRARY})
