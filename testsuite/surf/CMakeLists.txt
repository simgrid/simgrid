cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/testsuite/surf/")

add_executable(lmm_usage "lmm_usage.c")
add_executable(maxmin_bench "maxmin_bench.c")
add_executable(surf_usage "surf_usage.c")
add_executable(surf_usage2 "surf_usage2.c")
add_executable(trace_usage "trace_usage.c")

### Add definitions for compile
target_link_libraries(lmm_usage simgrid m -fprofile-arcs) 
target_link_libraries(maxmin_bench simgrid m -fprofile-arcs) 
target_link_libraries(surf_usage simgrid m -fprofile-arcs) 
target_link_libraries(surf_usage2 simgrid m -fprofile-arcs) 
target_link_libraries(trace_usage simgrid m -fprofile-arcs) 
