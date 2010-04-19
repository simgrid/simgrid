cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/tools/gras/")

add_executable(gras_stub_generator ${PROJECT_DIRECTORY}/tools/gras/stub_generator.c ${PROJECT_DIRECTORY}/tools/gras/unix_stub_generator.c ${PROJECT_DIRECTORY}/tools/gras/windows_stub_generator.c)

### Add definitions for compile
target_link_libraries(gras_stub_generator simgrid pthread m -fprofile-arcs)

### make files after the build
#ping
add_custom_command(TARGET gras_stub_generator
POST_BUILD
COMMAND "${PROJECT_DIRECTORY}/tools/gras/gras_stub_generator" 
ARGS ping ${PROJECT_DIRECTORY}/examples/gras/ping/ping.xml
WORKING_DIRECTORY "${PROJECT_DIRECTORY}/examples/gras/ping"
)
#rpc
add_custom_command(TARGET gras_stub_generator
POST_BUILD
COMMAND "${PROJECT_DIRECTORY}/tools/gras/gras_stub_generator" 
ARGS rpc ${PROJECT_DIRECTORY}/examples/gras/rpc/rpc.xml
WORKING_DIRECTORY "${PROJECT_DIRECTORY}/examples/gras/rpc"
)
#spawn
add_custom_command(TARGET gras_stub_generator
POST_BUILD
COMMAND "${PROJECT_DIRECTORY}/tools/gras/gras_stub_generator" 
ARGS --extra-process=child spawn ${PROJECT_DIRECTORY}/examples/gras/spawn/spawn.xml
WORKING_DIRECTORY "${PROJECT_DIRECTORY}/examples/gras/spawn"
)
#timer
add_custom_command(TARGET gras_stub_generator
POST_BUILD
COMMAND "${PROJECT_DIRECTORY}/tools/gras/gras_stub_generator" 
ARGS timer ${PROJECT_DIRECTORY}/examples/gras/timer/timer.xml
WORKING_DIRECTORY "${PROJECT_DIRECTORY}/examples/gras/timer"
)
#chrono
add_custom_command(TARGET gras_stub_generator
POST_BUILD
COMMAND "${PROJECT_DIRECTORY}/tools/gras/gras_stub_generator" 
ARGS chrono ${PROJECT_DIRECTORY}/examples/gras/chrono/chrono.xml
WORKING_DIRECTORY "${PROJECT_DIRECTORY}/examples/gras/chrono"
)
#simple_token
add_custom_command(TARGET gras_stub_generator
POST_BUILD
COMMAND "${PROJECT_DIRECTORY}/tools/gras/gras_stub_generator" 
ARGS simple_token ${PROJECT_DIRECTORY}/examples/gras/mutual_exclusion/simple_token/simple_token.xml
WORKING_DIRECTORY "${PROJECT_DIRECTORY}/examples/gras/mutual_exclusion/simple_token"
)
#mmrpc
add_custom_command(TARGET gras_stub_generator
POST_BUILD
COMMAND "${PROJECT_DIRECTORY}/tools/gras/gras_stub_generator" 
ARGS mmrpc ${PROJECT_DIRECTORY}/examples/gras/mmrpc/mmrpc.xml
WORKING_DIRECTORY "${PROJECT_DIRECTORY}/examples/gras/mmrpc"
)
#all2all
add_custom_command(TARGET gras_stub_generator
POST_BUILD
COMMAND "${PROJECT_DIRECTORY}/tools/gras/gras_stub_generator" 
ARGS all2all ${PROJECT_DIRECTORY}/examples/gras/all2all/all2all.xml
WORKING_DIRECTORY "${PROJECT_DIRECTORY}/examples/gras/all2all"
)
#pmm
add_custom_command(TARGET gras_stub_generator
POST_BUILD
COMMAND "${PROJECT_DIRECTORY}/tools/gras/gras_stub_generator" 
ARGS pmm ${PROJECT_DIRECTORY}/examples/gras/pmm/pmm.xml
WORKING_DIRECTORY "${PROJECT_DIRECTORY}/examples/gras/pmm"
)
#synchro
add_custom_command(TARGET gras_stub_generator
POST_BUILD
COMMAND "${PROJECT_DIRECTORY}/tools/gras/gras_stub_generator" 
ARGS synchro ${PROJECT_DIRECTORY}/examples/gras/synchro/synchro.xml
WORKING_DIRECTORY "${PROJECT_DIRECTORY}/examples/gras/synchro"
)
#properties
add_custom_command(TARGET gras_stub_generator
POST_BUILD
COMMAND "${PROJECT_DIRECTORY}/tools/gras/gras_stub_generator" 
ARGS properties ${PROJECT_DIRECTORY}/examples/gras/properties/properties.xml
WORKING_DIRECTORY "${PROJECT_DIRECTORY}/examples/gras/properties"
)
#teshsuite/gras/msg_handle
add_custom_command(TARGET gras_stub_generator
POST_BUILD
COMMAND "${PROJECT_DIRECTORY}/tools/gras/gras_stub_generator" 
ARGS msg_handle ${PROJECT_DIRECTORY}/teshsuite/gras/msg_handle/msg_handle.xml
WORKING_DIRECTORY "${PROJECT_DIRECTORY}/teshsuite/gras/msg_handle"
)
#teshsuite/gras/empty_main
add_custom_command(TARGET gras_stub_generator
POST_BUILD
COMMAND "${PROJECT_DIRECTORY}/tools/gras/gras_stub_generator" 
ARGS empty_main ${PROJECT_DIRECTORY}/teshsuite/gras/empty_main/empty_main.xml
WORKING_DIRECTORY "${PROJECT_DIRECTORY}/teshsuite/gras/empty_main"
)
#teshsuite/gras/small_sleep
add_custom_command(TARGET gras_stub_generator
POST_BUILD
COMMAND "${PROJECT_DIRECTORY}/tools/gras/gras_stub_generator" 
ARGS small_sleep ${PROJECT_DIRECTORY}/teshsuite/gras/small_sleep/small_sleep.xml
WORKING_DIRECTORY "${PROJECT_DIRECTORY}/teshsuite/gras/small_sleep"
)
#examples/amok/bandwidth
add_custom_command(TARGET gras_stub_generator
POST_BUILD
COMMAND "${PROJECT_DIRECTORY}/tools/gras/gras_stub_generator" 
ARGS bandwidth ${PROJECT_DIRECTORY}/examples/amok/bandwidth/bandwidth.xml
WORKING_DIRECTORY "${PROJECT_DIRECTORY}/examples/amok/bandwidth"
)
#examples/amok/saturate
add_custom_command(TARGET gras_stub_generator
POST_BUILD
COMMAND "${PROJECT_DIRECTORY}/tools/gras/gras_stub_generator" 
ARGS saturate ${PROJECT_DIRECTORY}/examples/amok/saturate/saturate.xml
WORKING_DIRECTORY "${PROJECT_DIRECTORY}/examples/amok/saturate"
)
