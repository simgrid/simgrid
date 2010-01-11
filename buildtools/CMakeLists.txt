cmake_minimum_required(VERSION 2.6)
SET(PROJECT_NAME "Simgrid_test")

### need to set
SET(disable_java 0)
SET(disable_gtnets 1)
SET(disable_context_threads 1)

### Set some variables
SET(LANGUAGE "Cxx")
SET(PROJECT_DIRECTORY "/home/navarrop/Bureau/simgrid-trunk")
SET(PROJECT_SOURCE_DIRECTORY "${PROJECT_DIRECTORY}/src")
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/bin")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")
set(INCLUDES "-I${PROJECT_DIRECTORY}/include -I${PROJECT_DIRECTORY}/src/include -I${PROJECT_DIRECTORY}/src")

### Initialize of CONTEXT
if(${disable_java})
  	SET(HAVE_JAVA 0)
else(${disable_java})
	include(FindJava)
	include(FindJNI)
	find_path(JAR	NAMES jar	PATHS NO_DEFAULT_PATHS	)	
	if(JAVA_RUNTIME AND JAVA_COMPILE AND JAR AND JAVA_INCLUDE_PATH)
		SET(HAVE_JAVA 1)
	else(JAVA_RUNTIME AND JAVA_COMPILE AND JAR AND JAVA_INCLUDE_PATH) 
		SET(HAVE_JAVA 0)
	endif(JAVA_RUNTIME AND JAVA_COMPILE AND JAR AND JAVA_INCLUDE_PATH)
endif(${disable_java})

if(${disable_gtnets})
	SET(HAVE_GTNETS 0)
else(${disable_gtnets})
	SET(HAVE_GTNETS 1)
	#AJOUT DES FLAGS GTNETS
endif(${disable_gtnets})

if(${disable_context_threads})
	SET(CONTEXT_THREADS 0)
else(${disable_context_threads})
	SET(CONTEXT_THREADS 1)
endif(${disable_context_threads})

message("HAVE_JAVA ${HAVE_JAVA}")
message("HAVE_GTNETS ${HAVE_GTNETS}")
message("CONTEXT_THREADS ${CONTEXT_THREADS}")


### define source packages
set(EXTRA_DIST 
	portable.h
	xbt/mallocator_private.h
	xbt/dict_private.h
	xbt/heap_private.h
	xbt/fifo_private.h
	xbt/graph_private.h
	xbt/graphxml_parse.c
	xbt/graphxml.l
	xbt/graphxml.c
	xbt/graphxml.dtd
	xbt/log_private.h
	xbt/ex_interface.h
	xbt/backtrace_linux.c
	xbt/backtrace_windows.c
	xbt/backtrace_dummy.c
	surf/maxmin_private.h
	surf/trace_mgr_private.h
	surf/surf_private.h
	surf/surfxml_parse.c
	surf/simgrid_dtd.l
	surf/simgrid_dtd.c
	surf/simgrid.dtd
	surf/network_private.h
	surf/network_gtnets_private.h
	surf/gtnets/gtnets_interface.h
	surf/gtnets/gtnets_simulator.h
	surf/gtnets/gtnets_topology.h
	surf/cpu_ti_private.h
	include/surf/surf_resource.h
	include/surf/datatypes.h
	include/surf/maxmin.h
	include/surf/trace_mgr.h
	include/surf/surf.h
	include/surf/surfxml_parse_private.h
	include/surf/random_mgr.h
	include/surf/surf_resource_lmm.h
	include/xbt/wine_dbghelp.h
	include/xbt/xbt_os_time.h
	include/xbt/xbt_os_thread.h
	msg/private.h
	msg/mailbox.h
	simdag/private.h
	simdag/dax.dtd
	simdag/dax_dtd.l
	simdag/dax_dtd.h
	simdag/dax_dtd.c
	gras/DataDesc/ddt_parse.yy.l
	gras/DataDesc/ddt_parse.yy.h
	gras/Virtu/virtu_rl.h
	gras/Virtu/virtu_sg.h
	gras/Virtu/virtu_interface.h
	gras/Virtu/virtu_private.h
	gras/Transport/transport_interface.h
	amok/Bandwidth/bandwidth_private.h
	amok/amok_modinter.h
	include/simix/simix.h
	include/simix/datatypes.h
	include/simix/context.h
	simix/private.h
	simix/smx_context_private.h
	simix/smx_context_java.h
	smpi/private.h
	smpi/smpi_coll_private.h
	smpi/smpi_mpi_dt_private.h
	smpi/README
	mk_supernovae.sh
)

set(XBT_RL_SRC 
	xbt/xbt_rl_synchro.c
	xbt/xbt_rl_time.c
)

set(XBT_SG_SRC 
	xbt/xbt_sg_synchro.c
	xbt/xbt_sg_time.c
)     

set(SMPI_SRC
	smpi/smpi_base.c
	smpi/smpi_bench.c
	smpi/smpi_global.c
	smpi/smpi_mpi.c
	smpi/smpi_sender.c
	smpi/smpi_receiver.c
	smpi/smpi_util.c
	smpi/smpi_coll.c
	smpi/smpi_mpi_dt.c
)

set(JMSG_C_SRC
	simix/smx_context_java.c
	java/jxbt_utilities.c
	java/jxbt_utilities.h
	java/jmsg.c java/jmsg.h
	java/jmsg_channel.c
	java/jmsg_channel.h
	java/jmsg_host.c
	java/jmsg_host.h
	java/jmsg_process.c
	java/jmsg_process.h
	java/jmsg_task.c
	java/jmsg_task.h
	java/jmsg_application_handler.c
	java/jmsg_application_handler.h
)

set(JMSG_JAVA_SRC
	java/simgrid/msg/MsgException.java
	java/simgrid/msg/JniException.java
	java/simgrid/msg/NativeException.java
	java/simgrid/msg/HostNotFoundException.java
	java/simgrid/msg/ProcessNotFoundException.java
	java/simgrid/msg/Msg.java
	java/simgrid/msg/Process.java
	java/simgrid/msg/Host.java
	java/simgrid/msg/Task.java
	java/simgrid/msg/MsgNative.java
	java/simgrid/msg/DTDResolver.java
	java/simgrid/msg/ApplicationHandler.java
	java/simgrid/msg/Sem.java
)

set(GRAS_RL_SRC
	gras/rl_stubs.c
	xbt/xbt_os_thread.c
	gras/Transport/rl_transport.c
	gras/Transport/transport_plugin_file.c
	gras/Transport/transport_plugin_tcp.c
	gras/Virtu/rl_emul.c
	gras/Virtu/rl_process.c
	gras/Virtu/rl_dns.c
	gras/Msg/rl_msg.c
	${XBT_RL_SRC}
)

set(XBT_SRC 
	xbt/snprintf.c
	xbt/xbt_str.c
	xbt/xbt_strbuff.c
	xbt/xbt_sha.c
	xbt/ex.c
	xbt_modinter.h
	gras_modinter.h
	xbt/xbt_virtu.c
	xbt/xbt_os_time.c
	xbt/asserts.c
	xbt/log.c
	xbt/xbt_log_appender_file.c
	xbt/xbt_log_layout_simple.c
	xbt/xbt_log_layout_format.c
	xbt/mallocator.c
	xbt/dynar.c
	xbt/dict.c
	xbt/dict_elm.c
	xbt/dict_cursor.c
	xbt/dict_multi.c
	xbt/heap.c
	xbt/fifo.c
	xbt/swag.c
	xbt/graph.c
	xbt/set.c
	xbt/xbt_matrix.c
	xbt/xbt_queue.c
	xbt/xbt_synchro.c
	xbt/xbt_peer.c
	xbt/xbt_main.c
	xbt/config.c
	xbt/cunit.c
	xbt/graphxml_parse.c
)

set(GTNETS_SRC 
	surf/gtnets/gtnets_simulator.cc
	surf/gtnets/gtnets_topology.cc
	surf/gtnets/gtnets_interface.cc
	surf/network_gtnets.c
)

set(SURF_SRC 
	surf/surf_model.c
	surf/surf_action.c
	surf/surf_routing.c
	surf/surf_config.c
	surf/maxmin.c
	surf/fair_bottleneck.c
	surf/lagrange.c
	surf/trace_mgr.c
	surf/random_mgr.c
	surf/surf.c
	surf/surfxml_parse.c
	surf/cpu.c
	surf/network.c
	surf/network_constant.c
	surf/workstation.c
	surf/surf_model_timer.c
	surf/workstation_ptask_L07.c
	surf/cpu_ti.c
	surf/cpu_im.c
	xbt/xbt_sg_stubs.c
)

set(SIMIX_SRC 
	simix/smx_global.c
	simix/smx_deployment.c
	simix/smx_environment.c
	simix/smx_host.c
	simix/smx_process.c
	simix/smx_context.c
	simix/smx_action.c
	simix/smx_synchro.c
	simix/smx_network.c
)

set(MSG_SRC
	msg/msg_config.c
	msg/task.c
	msg/host.c
	msg/m_process.c
	msg/gos.c
	msg/global.c
	msg/environment.c
	msg/deployment.c
	msg/msg_mailbox.c
	msg/msg_actions.c
)

set(SIMDAG_SRC
	simdag/sd_global.c
	simdag/sd_link.c
	simdag/sd_task.c
	simdag/sd_workstation.c
	simdag/sd_daxloader.c
)

set(GRAS_COMMON_SRC
	gras/gras.c
	gras/Transport/transport.c
	gras/Transport/transport_private.h
	gras/Msg/gras_msg_mod.c
	gras/Msg/gras_msg_types.c
	gras/Msg/gras_msg_exchange.c
	gras/Msg/gras_msg_listener.c
	gras/Msg/rpc.c gras/Msg/timer.c
	gras/Msg/msg_interface.h
	gras/Msg/msg_private.h
	gras/Virtu/process.c
	gras/Virtu/gras_module.c
	gras/DataDesc/ddt_create.c
	gras/DataDesc/ddt_convert.c
	gras/DataDesc/ddt_exchange.c
	gras/DataDesc/cbps.c
	gras/DataDesc/datadesc.c
	gras/DataDesc/datadesc_interface.h
	gras/DataDesc/datadesc_private.h
	gras/DataDesc/ddt_parse.c
	gras/DataDesc/ddt_parse.yy.c
)

set(GRAS_SG_SRC
	gras/Transport/sg_transport.c
	gras/Transport/transport_plugin_sg.c
	gras/Virtu/sg_emul.c
	gras/Virtu/sg_process.c
	gras/Virtu/sg_dns.c
	gras/Msg/sg_msg.c
	${XBT_SG_SRC}
)

set(AMOK_SRC
	amok/amok_base.c
	amok/Bandwidth/bandwidth.c
	amok/Bandwidth/saturate.c
	amok/PeerManagement/peermanagement.c
)

### depend of some variables setted upper
# -->CONTEXT_THREADS
if(${CONTEXT_THREADS})
	set(SURF_SRC
		${SURF_SRC}
		xbt/xbt_os_thread.c
		simix/smx_context_thread.c
	)
	set(EXTRA_DIST
		${EXTRA_DIST}
		simix/smx_context_sysv.c
	)
else(${CONTEXT_THREADS})
	set(SURF_SRC
		${SURF_SRC}
		simix/smx_context_sysv.c
	)
	set(EXTRA_DIST
		${EXTRA_DIST}
		xbt/xbt_os_thread.c
		simix/smx_context_thread.c
	)
endif(${CONTEXT_THREADS})

# -->HAVE_GTNETS
if(${HAVE_GTNETS})
  	set(GTNETS_USED 
		${GTNETS_SRC}
	)
else(${HAVE_GTNETS})
  	set(GTNETS_USED "")
	set(EXTRA_DIST
		${EXTRA_DIST}
		${GTNETS_SRC}
	)
endif(${HAVE_GTNETS})

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
	${SIMDAG_SRC}
	${GRAS_COMMON_SRC}
	${GRAS_SG_SRC}
	${AMOK_SRC}
)

### Gras Lib sources
set(gras_sources
	${XBT_SRC}
	${GRAS_COMMON_SRC}
	${GRAS_RL_SRC}
	${AMOK_SRC}
)

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

#message("\n\n")
#message("simgrid_sources\n>>${simgrid_sources}<<")
#message("\n\n")
#message("gras_sources\n>>${gras_sources}<<")
#message("\n\n")
#message("surf_sources\n>>${SURF_SRC}<<")
#message("\n\n")
#message("EXTRA_DIST\n>>${EXTRA_DIST}<<")
#message("\n\n")

add_definitions("${INCLUDES}")

add_library(simgrid SHARED ${simgrid_sources})
add_library(gras SHARED ${gras_sources})









