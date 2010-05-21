### define source packages

set(EXTRA_DIST 
	src/portable.h
	src/xbt/mallocator_private.h
	src/xbt/dict_private.h
	src/xbt/heap_private.h
	src/xbt/fifo_private.h
	src/xbt/graph_private.h
	src/xbt/graphxml_parse.c
	src/xbt/graphxml.l
	src/xbt/graphxml.c
	src/xbt/graphxml.dtd
	src/xbt/log_private.h
	src/xbt/ex_interface.h
	src/xbt/backtrace_linux.c
	src/xbt/backtrace_windows.c
	src/xbt/backtrace_dummy.c
	src/xbt/setset_private.h
	src/xbt/mmalloc/attach.c
	src/xbt/mmalloc/detach.c	
	src/xbt/mmalloc/keys.c
	src/xbt/mmalloc/mcalloc.c
	src/xbt/mmalloc/mfree.c
	src/xbt/mmalloc/mm_legacy.c		
	src/xbt/mmalloc/mm.c
	src/xbt/mmalloc/mmalloc.c
	src/xbt/mmalloc/mmap-sup.c
	src/xbt/mmalloc/mmcheck.c
	src/xbt/mmalloc/mmemalign.c
	src/xbt/mmalloc/mmprivate.h
	src/xbt/mmalloc/mmstats.c
	src/xbt/mmalloc/mmtrace.c
	src/xbt/mmalloc/mrealloc.c
	src/xbt/mmalloc/mvalloc.c
	src/xbt/mmalloc/sbrk-sup.c
	src/xbt/mmalloc/test/mmalloc_test.c
	src/surf/maxmin_private.h
	src/surf/trace_mgr_private.h
	src/surf/surf_private.h
	src/surf/surfxml_parse.c
	src/surf/simgrid_dtd.l
	src/surf/simgrid_dtd.c
	src/surf/simgrid.dtd
	src/surf/network_private.h
	src/surf/network_gtnets_private.h
	src/surf/gtnets/gtnets_interface.h
	src/surf/gtnets/gtnets_simulator.h
	src/surf/gtnets/gtnets_topology.h
	src/surf/cpu_ti_private.h
	src/include/surf/surf_resource.h
	src/include/surf/datatypes.h
	src/include/surf/maxmin.h
	src/include/surf/trace_mgr.h
	src/include/surf/surf.h
	src/include/surf/surfxml_parse_private.h
	src/include/surf/random_mgr.h
	src/include/surf/surf_resource_lmm.h
	src/include/xbt/wine_dbghelp.h
	src/include/xbt/xbt_os_time.h
	src/include/xbt/xbt_os_thread.h
	src/include/mc/datatypes.h
	src/include/mc/mc.h
	src/include/simix/simix.h
	src/include/simix/datatypes.h
	src/include/simix/context.h
	src/msg/private.h
	src/msg/mailbox.h
	src/simdag/private.h
	src/simdag/dax.dtd
	src/simdag/dax_dtd.l
	src/simdag/dax_dtd.h
	src/simdag/dax_dtd.c
	src/gras/DataDesc/ddt_parse.yy.l
	src/gras/DataDesc/ddt_parse.yy.h
	src/gras/Virtu/virtu_rl.h
	src/gras/Virtu/virtu_sg.h
	src/gras/Virtu/virtu_interface.h
	src/gras/Virtu/virtu_private.h
	src/gras/Transport/transport_interface.h
	src/amok/Bandwidth/bandwidth_private.h
	src/amok/amok_modinter.h
	src/simix/private.h
	src/simix/smx_context_private.h
	src/simix/smx_context_java.h
	src/smpi/private.h
	src/smpi/smpi_coll_private.h
	src/smpi/smpi_mpi_dt_private.h
	src/smpi/README
	src/mk_supernovae.sh
)

set(XBT_RL_SRC 
	src/xbt/xbt_rl_synchro.c
	src/xbt/xbt_rl_time.c
)

set(XBT_SG_SRC 
	src/xbt/xbt_sg_synchro.c
	src/xbt/xbt_sg_time.c
)     

set(SMPI_SRC
	src/smpi/smpi_base.c
	src/smpi/smpi_bench.c
	src/smpi/smpi_global.c
	src/smpi/smpi_mpi.c
	src/smpi/smpi_comm.c
	src/smpi/smpi_group.c
	src/smpi/smpi_util.c
	src/smpi/smpi_coll.c
	src/smpi/smpi_mpi_dt.c
)

set(JMSG_C_SRC
	src/simix/smx_context_java.c
	src/java/jxbt_utilities.c
	src/java/jxbt_utilities.h
	src/java/jmsg.c 
	src/java/jmsg.h
	src/java/jmsg_host.c
	src/java/jmsg_host.h
	src/java/jmsg_process.c
	src/java/jmsg_process.h
	src/java/jmsg_task.c
	src/java/jmsg_task.h
	src/java/jmsg_application_handler.c
	src/java/jmsg_application_handler.h
)

set(JMSG_JAVA_SRC
	src/java/simgrid/msg/ApplicationHandler.java
	src/java/simgrid/msg/Host.java
	src/java/simgrid/msg/HostFailureException.java	
	src/java/simgrid/msg/HostNotFoundException.java	
	src/java/simgrid/msg/JniException.java
	src/java/simgrid/msg/Msg.java
	src/java/simgrid/msg/MsgException.java
	src/java/simgrid/msg/MsgNative.java
	src/java/simgrid/msg/NativeException.java
	src/java/simgrid/msg/Process.java
	src/java/simgrid/msg/ProcessNotFoundException.java
	src/java/simgrid/msg/Sem.java
	src/java/simgrid/msg/Task.java
	src/java/simgrid/msg/TaskCancelledException.java
	src/java/simgrid/msg/TimeoutException.java
	src/java/simgrid/msg/TransferFailureException.java	
)

set(GRAS_RL_SRC
	src/gras/rl_stubs.c
	src/xbt/xbt_os_thread.c
	src/gras/Transport/rl_transport.c
	src/gras/Transport/transport_plugin_file.c
	src/gras/Transport/transport_plugin_tcp.c
	src/gras/Virtu/rl_emul.c
	src/gras/Virtu/rl_process.c
	src/gras/Virtu/rl_dns.c
	src/gras/Msg/rl_msg.c
	${XBT_RL_SRC}
)

set(XBT_SRC 
	src/xbt/snprintf.c
	src/xbt/xbt_str.c
	src/xbt/xbt_strbuff.c
	src/xbt/xbt_sha.c
	src/xbt/ex.c
	src/xbt_modinter.h
	src/gras_modinter.h
	src/xbt/xbt_virtu.c
	src/xbt/xbt_os_time.c
	src/xbt/asserts.c
	src/xbt/log.c
	src/xbt/xbt_log_appender_file.c
	src/xbt/xbt_log_layout_simple.c
	src/xbt/xbt_log_layout_format.c
	src/xbt/mallocator.c
	src/xbt/dynar.c
	src/xbt/dict.c
	src/xbt/dict_elm.c
	src/xbt/dict_cursor.c
	src/xbt/dict_multi.c
	src/xbt/heap.c
	src/xbt/fifo.c
	src/xbt/swag.c
	src/xbt/graph.c
	src/xbt/set.c
	src/xbt/xbt_matrix.c
	src/xbt/xbt_queue.c
	src/xbt/xbt_synchro.c
	src/xbt/xbt_peer.c
	src/xbt/xbt_main.c
	src/xbt/config.c
	src/xbt/cunit.c
	src/xbt/graphxml_parse.c
	src/xbt/setset.c
	src/xbt/mmalloc/mm.c
)

set(GTNETS_SRC 
	src/surf/gtnets/gtnets_simulator.cc
	src/surf/gtnets/gtnets_topology.cc
	src/surf/gtnets/gtnets_interface.cc
	src/surf/network_gtnets.c
)

set(SURF_SRC 
	src/surf/surf_model.c
	src/surf/surf_action.c
	src/surf/surf_routing.c
	src/surf/surf_config.c
	src/surf/maxmin.c
	src/surf/fair_bottleneck.c
	src/surf/lagrange.c
	src/surf/trace_mgr.c
	src/surf/random_mgr.c
	src/surf/surf.c
	src/surf/surfxml_parse.c
	src/surf/cpu.c
	src/surf/network.c
	src/surf/network_vivaldi.c
	src/surf/network_constant.c
	src/surf/workstation.c
	src/surf/surf_model_timer.c
	src/surf/workstation_ptask_L07.c
	src/surf/cpu_ti.c
	src/surf/cpu_im.c
	src/xbt/xbt_sg_stubs.c
)

set(SIMIX_SRC 
	src/simix/smx_global.c
	src/simix/smx_deployment.c
	src/simix/smx_environment.c
	src/simix/smx_host.c
	src/simix/smx_process.c
	src/simix/smx_context.c
	src/simix/smx_action.c
	src/simix/smx_synchro.c
	src/simix/smx_network.c
	src/simix/smx_context_base.c
)

set(MSG_SRC
	src/msg/msg_config.c
	src/msg/task.c
	src/msg/host.c
	src/msg/m_process.c
	src/msg/gos.c
	src/msg/global.c
	src/msg/environment.c
	src/msg/deployment.c
	src/msg/msg_mailbox.c
	src/msg/msg_actions.c
)

set(SIMDAG_SRC
	src/simdag/sd_global.c
	src/simdag/sd_link.c
	src/simdag/sd_task.c
	src/simdag/sd_workstation.c
	src/simdag/sd_daxloader.c
)

set(GRAS_COMMON_SRC
	src/gras/gras.c
	src/gras/Transport/transport.c
	src/gras/Transport/transport_private.h
	src/gras/Msg/gras_msg_mod.c
	src/gras/Msg/gras_msg_types.c
	src/gras/Msg/gras_msg_exchange.c
	src/gras/Msg/gras_msg_listener.c
	src/gras/Msg/rpc.c 
	src/gras/Msg/timer.c
	src/gras/Msg/msg_interface.h
	src/gras/Msg/msg_private.h
	src/gras/Virtu/process.c
	src/gras/Virtu/gras_module.c
	src/gras/DataDesc/ddt_create.c
	src/gras/DataDesc/ddt_convert.c
	src/gras/DataDesc/ddt_exchange.c
	src/gras/DataDesc/cbps.c
	src/gras/DataDesc/datadesc.c
	src/gras/DataDesc/datadesc_interface.h
	src/gras/DataDesc/datadesc_private.h
	src/gras/DataDesc/ddt_parse.c
	src/gras/DataDesc/ddt_parse.yy.c
)

set(GRAS_SG_SRC
	src/gras/Transport/sg_transport.c
	src/gras/Transport/transport_plugin_sg.c
	src/gras/Virtu/sg_emul.c
	src/gras/Virtu/sg_process.c
	src/gras/Virtu/sg_dns.c
	src/gras/Msg/sg_msg.c
	${XBT_SG_SRC}
)

set(AMOK_SRC
	src/amok/amok_base.c
	src/amok/Bandwidth/bandwidth.c
	src/amok/Bandwidth/saturate.c
	src/amok/PeerManagement/peermanagement.c
)

set(LUA_SRC
	src/simix/smx_context_lua.c
	src/bindings/lua/simgrid_lua.c
)

set(TRACING_SRC
	src/instr/interface.c
	src/instr/general.c
	src/instr/paje.c
	src/instr/msg_task_instr.c
	src/instr/msg_process_instr.c
	src/instr/msg_volume.c
	src/instr/smx_instr.c
	src/instr/surf_instr.c
	src/instr/variables_instr.c
	src/instr/private.h
)

set(RUBY_SRC
src/simix/smx_context_ruby.c
src/bindings/ruby/rb_msg_process.c
src/bindings/ruby/rb_msg_host.c
src/bindings/ruby/rb_msg_task.c
src/bindings/ruby/rb_application_handler.c
)

set(MC_SRC
	src/mc/mc_memory.c
	src/mc/mc_checkpoint.c
	src/mc/memory_map.c
	src/mc/mc_global.c
	src/mc/mc_dfs.c
	src/mc/mc_dpor.c
	src/mc/mc_transition.c
	src/mc/private.h
)


set(install_HEADERS
include/gras.h 
include/xbt.h
include/simgrid_config.h
include/xbt/misc.h
include/xbt/sysdep.h
include/xbt/virtu.h
include/xbt/str.h
include/xbt/strbuff.h
include/xbt/hash.h
include/xbt/function_types.h
include/xbt/asserts.h 
include/xbt/ex.h
include/xbt/log.h
include/xbt/module.h
include/xbt/mallocator.h
include/xbt/dynar.h
include/xbt/dict.h
include/xbt/set.h
include/xbt/heap.h
include/xbt/graph.h
include/xbt/fifo.h
include/xbt/swag.h
include/xbt/matrix.h
include/xbt/peer.h
include/xbt/config.h
include/xbt/cunit.h
include/xbt/graphxml_parse.h
include/xbt/graphxml.h
include/xbt/time.h
include/xbt/synchro.h
include/xbt/synchro_core.h
include/xbt/queue.h
include/xbt/setset.h
include/xbt/mmalloc.h
include/mc/modelchecker.h
include/msg/msg.h
include/msg/datatypes.h
include/simdag/simdag.h
include/simdag/datatypes.h
include/smpi/smpi.h
include/smpi/mpi.h
include/surf/surfxml_parse.h
include/surf/simgrid_dtd.h
include/gras/datadesc.h
include/gras/transport.h
include/gras/virtu.h
include/gras/emul.h
include/gras/process.h
include/gras/module.h
include/gras/messages.h
include/gras/timer.h
include/amok/peermanagement.h
include/amok/bandwidth.h
include/instr/instr.h
include/instr/tracing_config.h
)

set(TEST_UNITS
cunit_unit.c
ex_unit.c
dynar_unit.c
dict_unit.c
set_unit.c
swag_unit.c
xbt_str_unit.c
xbt_strbuff_unit.c
xbt_sha_unit.c
config_unit.c
xbt_synchro_unit.c
)

set(TEST_CFILES
src/xbt/cunit.c
src/xbt/ex.c
src/xbt/dynar.c
src/xbt/dict.c
src/xbt/set.c
src/xbt/swag.c
src/xbt/xbt_str.c
src/xbt/xbt_strbuff.c
src/xbt/xbt_sha.c
src/xbt/config.c
src/xbt/xbt_synchro.c
)

#Here must have all files which permit to generate source files
set(SRC_TO_LOOK
src/surf/simgrid.dtd
src/xbt/graphxml.dtd
src/simdag/dax.dtd
examples/gras/ping/ping.xml
examples/gras/rpc/rpc.xml
examples/gras/spawn/spawn.xml
examples/gras/timer/timer.xml
examples/gras/chrono/chrono.xml
examples/gras/mutual_exclusion/simple_token/simple_token.xml
examples/gras/mmrpc/mmrpc.xml
examples/gras/all2all/all2all.xml
examples/gras/pmm/pmm.xml
examples/gras/synchro/synchro.xml
examples/gras/properties/properties.xml
teshsuite/gras/msg_handle/msg_handle.xml
teshsuite/gras/empty_main/empty_main.xml
teshsuite/gras/small_sleep/small_sleep.xml
examples/amok/bandwidth/bandwidth.xml
examples/amok/saturate/saturate.xml
${TEST_CFILES}
)

### depend of some variables setted upper
# -->CONTEXT_THREADS
if(${CONTEXT_THREADS})
	set(SURF_SRC
		${SURF_SRC}
		src/xbt/xbt_os_thread.c
		src/simix/smx_context_thread.c
	)
	set(EXTRA_DIST
		${EXTRA_DIST}
		src/simix/smx_context_sysv.c
	)
else(${CONTEXT_THREADS})
	set(SURF_SRC
		${SURF_SRC}
		src/simix/smx_context_sysv.c
	)
	set(EXTRA_DIST
		${EXTRA_DIST}
		src/xbt/xbt_os_thread.c
		src/simix/smx_context_thread.c
	)
endif(${CONTEXT_THREADS})

# -->HAVE_GTNETS
if(HAVE_GTNETS)
  	set(GTNETS_USED 
		${GTNETS_SRC}
	)
else(HAVE_GTNETS)
  	set(GTNETS_USED "")
	set(EXTRA_DIST
		${EXTRA_DIST}
		${GTNETS_SRC}
	)
endif(HAVE_GTNETS)

set(EXTRA_DIST
	${EXTRA_DIST}
	${JMSG_JAVA_SRC}
)

### Simgrid Lib sources
set(simgrid_sources
	${XBT_SRC}
	${SURF_SRC}
	${GTNETS_USED}
	${SIMIX_SRC}
	${MSG_SRC}
	${TRACING_SRC}
	${SIMDAG_SRC}
	${GRAS_COMMON_SRC}
	${GRAS_SG_SRC}
	${AMOK_SRC}
	${MC_SRC}
)

### Gras Lib sources
set(gras_sources
	${XBT_SRC}
	${GRAS_COMMON_SRC}
	${GRAS_RL_SRC}
	${AMOK_SRC}
)

if(${HAVE_LUA})
  	set(simgrid_sources
		${simgrid_sources}
		${LUA_SRC}
	)
elseif(${HAVE_LUA})
  	set(EXTRA_DIST
		${EXTRA_DIST}
		${LUA_SRC}
	)
endif(${HAVE_LUA})

if(${HAVE_JAVA})
	set(simgrid_sources
		${simgrid_sources}
		${JMSG_C_SRC} # add the binding support to the library
	)
else(${HAVE_JAVA})
	set(EXTRA_DIST
		${EXTRA_DIST}
		${JMSG_C_SRC}
		${MSG_SRC}
	)
endif(${HAVE_JAVA})

if(${HAVE_RUBY})
	set(simgrid_sources
		${simgrid_sources}
		${RUBY_SRC}
		src/bindings/ruby/simgrid_ruby.c
	)
else(${HAVE_RUBY})
	set(EXTRA_DIST
		${EXTRA_DIST}
		${RUBY_SRC}
		src/bindings/ruby/simgrid_ruby.c
	)
endif(${HAVE_RUBY})

file(GLOB_RECURSE add_src_files
"examples/*.c"
"teshsuite/*.c"
"testsuite/*.c"
"tools/*.c"
"examples/*.cxx"
"teshsuite/*.cxx"
"testsuite/*.cxx"
"tools/*.cxx"
"examples/*CMakeLists.txt"
"teshsuite/*CMakeLists.txt"
"testsuite/*CMakeLists.txt"
"tools/*CMakeLists.txt"
"src/*CMakeLists.txt"
"examples/*.java"
)

file(GLOB_RECURSE examples_to_install_in_doc
"examples/*.c"
"examples/*.h"
"examples/*.cxx"
"examples/*.hpp"
"examples/*.c"
"examples/*.rb"
"examples/*.lua"
"examples/*.java"
"examples/*.xml"
)

foreach(file ${examples_to_install_in_doc})
	string(REGEX REPLACE "/[^/]*$" "" file "${file}")
	set(new_examples_to_install_in_doc "${new_examples_to_install_in_doc}${file};")
endforeach(file ${examples_to_install_in_doc})

set(directory_to_create "")

foreach(file ${new_examples_to_install_in_doc})
	string(REGEX MATCH "${file};" OPERATION "${directory_to_create}")
	if(NOT OPERATION)
		set(directory_to_create "${directory_to_create}${file};")
	endif(NOT OPERATION)
endforeach(file ${new_examples_to_install_in_doc})


file(GLOB_RECURSE include_files
"include/*.h"
"teshsuite/*.h"
"testsuite/*.h"
"tools/*.h"
"examples/*.h"
"examples/*.hpp"
"src/*.h.in"
"include/*.h.in"
)
#message("\n\ninclude:\n${include_files}")

file(GLOB_RECURSE xml_files
"examples/*.xml"
"include/*.xml"
"src/*.xml"
"teshsuite/*.xml"
"testsuite/*.xml"
"tools/*.xml"
)
#message("\n\nxml:\n${xml_files}")

file(GLOB_RECURSE tesh_files
"examples/*.tesh"
"include/*.tesh"
"src/*.tesh"
"teshsuite/*.tesh"
"testsuite/*.tesh"
"tools/*.tesh"
)
#message("\n\ntesh:\n${tesh_files}")

file(GLOB_RECURSE txt_files
"testsuite/surf/trace*.txt"
"testsuite/simdag/availability_tremblay.txt"
"examples/smpi/hostfile"
"examples/msg/*.trace"
"examples/msg/migration/migration.deploy"
"examples/java/runtest"
"teshsuite/gras/datadesc/datadesc.little32_4"
"teshsuite/gras/datadesc/datadesc.little64"
"teshsuite/gras/datadesc/datadesc.big32_8"
"teshsuite/gras/datadesc/datadesc.big32_8_4"
"teshsuite/gras/datadesc/datadesc.big32_2"
"teshsuite/gras/datadesc/mk_datadesc_structs.pl"
"teshsuite/gras/msg_handle/test_rl"
"teshsuite/gras/msg_handle/test_sg_32"
"teshsuite/gras/msg_handle/test_sg_64"
"teshsuite/gras/empty_main/test_rl"
"teshsuite/gras/empty_main/test_sg"
"teshsuite/gras/small_sleep/test_sg_32"
"teshsuite/gras/small_sleep/test_sg_64"
"teshsuite/simdag/platforms/bob.fail"
"teshsuite/simdag/platforms/bob.trace"
"teshsuite/simdag/platforms/link1.bw"
"teshsuite/simdag/platforms/link1.fail"
"teshsuite/simdag/platforms/link1.lat"
"examples/gras/ping/test_rl"
"examples/gras/rpc/test_rl"
"examples/gras/spawn/test_rl"
"examples/gras/timer/test_rl"
"examples/gras/chrono/test_rl"
"examples/gras/mutual_exclusion/simple_token/test_rl"
"examples/gras/mmrpc/test_rl"
"examples/gras/all2all/test_rl"
"examples/gras/pmm/test_rl"
"examples/gras/synchro/test_rl"
"examples/gras/properties/test_rl"
"examples/gras/ping/test_sg_32"
"examples/gras/rpc/test_sg_32"
"examples/gras/spawn/test_sg_32"
"examples/gras/timer/test_sg_32"
"examples/gras/chrono/test_sg_32"
"examples/gras/mutual_exclusion/simple_token/test_sg_32"
"examples/gras/mmrpc/test_sg_32"
"examples/gras/all2all/test_sg_32"
"examples/gras/pmm/test_sg_32"
"examples/gras/synchro/test_sg_32"
"examples/gras/ping/test_sg_64"
"examples/gras/rpc/test_sg_64"
"examples/gras/spawn/test_sg_64"
"examples/gras/timer/test_sg_64"
"examples/gras/chrono/test_sg_64"
"examples/gras/mutual_exclusion/simple_token/test_sg_64"
"examples/gras/mmrpc/test_sg_64"
"examples/gras/all2all/test_sg_64"
"examples/gras/pmm/test_sg_64"
"examples/gras/synchro/test_sg_64"
"examples/gras/properties/test_sg"
"examples/java/basic/BasicTest"
"examples/java/ping_pong/PingPongTest"
"examples/java/comm_time/CommTimeTest"
"examples/java/suspend/SuspendTest"
)



# This is the complete lise of what will be added to the source archive
set(source_to_pack	
	${XBT_RL_SRC}
	${EXTRA_DIST}
	${SMPI_SRC}
	${JMSG_C_SRC}
	${JMSG_JAVA_SRC}
	${GRAS_RL_SRC}
	${XBT_SRC}
	${GTNETS_SRC}
	${SURF_SRC}
	${SIMIX_SRC}
	${TRACING_SRC}
	${MSG_SRC}
	${SIMDAG_SRC}
	${GRAS_COMMON_SRC}
	${GRAS_SG_SRC}
	${AMOK_SRC}
	${LUA_SRC}
	${RUBY_SRC}
	${MC_SRC}
	${add_src_files}
	${include_files}
	${xml_files}
	${tesh_files}
	${txt_files}
	${TEST_CFILES}
	${EXTRA_DIST}
	
	CMakeLists.txt
	buildtools/Cmake/CTestConfig.cmake
	buildtools/Cmake/CompleteInFiles.cmake
	buildtools/Cmake/DefinePackages.cmake
	buildtools/Cmake/Distrib.cmake
	buildtools/Cmake/GenerateDoc.cmake
	buildtools/Cmake/Flags.cmake
	buildtools/Cmake/MakeJava.cmake
	buildtools/Cmake/MaintainerMode.cmake
	buildtools/Cmake/MakeExeLib.cmake
	buildtools/Cmake/Option.cmake
	buildtools/Cmake/PrintArgs.cmake
	buildtools/Cmake/Supernovae.cmake
	buildtools/Cmake/AddTests.cmake
	buildtools/Cmake/memcheck_tests.cmake
	# FIXME: these should live in src/ and the content of src in root of Cmake/ maybe
	buildtools/Cmake/test_prog/prog_AC_CHECK_MCSC.c
	buildtools/Cmake/test_prog/prog_getline.c
	buildtools/Cmake/test_prog/prog_GRAS_ARCH.c
	buildtools/Cmake/test_prog/prog_GRAS_CHECK_STRUCT_COMPACTION.c
	buildtools/Cmake/test_prog/prog_gtnets.cpp
	buildtools/Cmake/test_prog/prog_max_size.c
	buildtools/Cmake/test_prog/prog_mutex_timedlock.c
	buildtools/Cmake/test_prog/prog_printf_null.c
	buildtools/Cmake/test_prog/prog_sem_init.c
	buildtools/Cmake/test_prog/prog_sem_timedwait.c
	buildtools/Cmake/test_prog/prog_snprintf.c
	buildtools/Cmake/test_prog/prog_stackgrowth.c
	buildtools/Cmake/test_prog/prog_stacksetup.c
	buildtools/Cmake/test_prog/prog_va_copy.c
	buildtools/Cmake/test_prog/prog_vsnprintf.c
	# FIXME: these are badly named and placed
	buildtools/Cmake/gras_config.h.in
	buildtools/Cmake/tracing_config.h.in
	
	AUTHORS
	ChangeLog
	COPYING
	missing
	NEWS
	README
	README.IEEE
	TODO
	src/smpi/smpicc.in
	src/smpi/smpirun.in
	src/bindings/ruby/simgrid_ruby.c
	src/bindings/ruby_bindings.h
	src/simix/smx_context_sysv_private.h
	src/simgrid_units_main.c
	src/cunit_unit.c
	src/ex_unit.c
	src/dynar_unit.c
	src/dict_unit.c
	src/set_unit.c
	src/swag_unit.c
	src/xbt_str_unit.c
	src/xbt_strbuff_unit.c
	src/xbt_sha_unit.c
	src/config_unit.c
	src/xbt_synchro_unit.c
	src/bindings/lua/master_slave.lua
	src/bindings/lua/mult_matrix.lua
	examples/lua/master_slave.lua
	examples/lua/mult_matrix.lua
	examples/lua/README
	src/bindings/ruby/MasterSlave.rb
	src/bindings/ruby/MasterSlaveData.rb
	src/bindings/ruby/PingPong.rb
	src/bindings/ruby/Quicksort.rb
	src/bindings/ruby/simgrid.rb
	examples/ruby/README
	examples/ruby/MasterSlave.rb
	examples/ruby/PingPong.rb
	examples/ruby/Quicksort.rb
	src/bindings/rubyDag/example.rb
	src/bindings/rubyDag/extconfig.rb
	src/bindings/rubyDag/rb_SD_task.c
	src/bindings/rubyDag/rb_SD_task.h
	src/bindings/rubyDag/rb_SD_workstation.c
	src/bindings/rubyDag/rb_SD_workstation.h
	src/bindings/rubyDag/rb_simdag.c
	src/bindings/rubyDag/simdag.rb
)
#set(script_to_install
#	src/smpi/smpicc
#	src/smpi/smpirun
#	tools/MSG_visualization/colorize.pl
#)
#	tools/sg_unit_extractor.pl
#	tools/doxygen/index_create.pl
#	tools/doxygen/toc_create.pl
#	tools/doxygen/index_php.pl
#	tools/doxygen/doxygen_postprocesser.pl
#	tools/doxygen/bibtex2html_table_count.pl
#	tools/doxygen/bibtex2html_postprocessor.pl
#	tools/doxygen/xbt_log_extract_hierarchy.pl
#	tools/MSG_visualization/trace2fig.pl

#message("\n\ntesh:\n${txt_files}")
