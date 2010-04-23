cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/examples/msg/tracing/")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")

add_executable(ms ${PROJECT_DIRECTORY}/examples/msg/tracing/ms.c)
add_executable(categories ${PROJECT_DIRECTORY}/examples/msg/tracing/categories.c)
add_executable(volume ${PROJECT_DIRECTORY}/examples/msg/tracing/volume.c)
add_executable(tasks ${PROJECT_DIRECTORY}/examples/msg/tracing/tasks.c)
add_executable(procmig ${PROJECT_DIRECTORY}/examples/msg/tracing/procmig.c)

### Add definitions for compile
target_link_libraries(ms simgrid m -fprofile-arcs)
target_link_libraries(categories simgrid m -fprofile-arcs)
target_link_libraries(volume simgrid m -fprofile-arcs)
target_link_libraries(tasks simgrid m -fprofile-arcs)
target_link_libraries(procmig simgrid m -fprofile-arcs)
