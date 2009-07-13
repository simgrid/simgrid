# Copyright (c) 2004-2007. The SimGrid team. All right reserved.

# This file is part of the SimGrid project. This is free software:
# You can redistribute and/or modify it under the terms of the
# GNU LGPL (v2.1) licence.


INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src -I$(top_srcdir)/src/include
AM_CFLAGS = -g $(INCLUDES)

# Declare all the variables to empty, so that each test set can add
# their content to them
CLEANFILES = 
EXTRA_DIST= 
noinst_PROGRAMS=

TESTS_ENVIRONMENT = $(top_builddir)/tools/tesh/tesh
TESTS =
XFAIL_TESTS=

LDADD_SG=$(abs_top_builddir)/src/libsimgrid.la
LDADD_RL=$(abs_top_builddir)/src/libgras.la

#################
### XBT tests ###
#################

  ## Try large logs ##
  ####################

TESTS += xbt/log_large_test.tesh
noinst_PROGRAMS += xbt/log_large_test
xbt_log_large_test_LDADD = $(LDADD_SG)

  ## Try parallel logs ##
  #######################

TESTS += xbt/parallel_log_crashtest.tesh
noinst_PROGRAMS += xbt/parallel_log_crashtest 
xbt_parallel_log_crashtest_LDADD = $(LDADD_RL)

##################
### GRAS tests ###
##################

  ## Try structured communication facilities ##
  #############################################

CLEANFILES += gras/datadesc/datadesc_usage.out
EXTRA_DIST += \
  gras/datadesc/datadesc.little32_4 \
  gras/datadesc/datadesc.little64\
  gras/datadesc/datadesc.big32_8 gras/datadesc/datadesc.big32_8_4 gras/datadesc/datadesc.big32_2 \
  gras/datadesc/mk_datadesc_structs.pl
TESTS += \
  gras/datadesc/datadesc_mem.tesh          \
  gras/datadesc/datadesc_rw.tesh           \
  gras/datadesc/datadesc_r_little32_4.tesh \
  gras/datadesc/datadesc_r_little64.tesh   \
  \
  gras/datadesc/datadesc_r_big32_8.tesh      \
  gras/datadesc/datadesc_r_big32_8_4.tesh  \
  gras/datadesc/datadesc_r_big32_2.tesh
# Data sets still to regenerate:
XFAIL_TESTS+= \
  gras/datadesc/datadesc_r_little32.tesh   \
  gras/datadesc/datadesc_r_big32_8.tesh    \
  gras/datadesc/datadesc_r_big32_2.tesh

noinst_PROGRAMS += gras/datadesc/datadesc_usage   
gras_datadesc_datadesc_usage_SOURCES=   gras/datadesc/datadesc_usage.c gras/datadesc/datadesc_structs.c
gras_datadesc_datadesc_usage_LDADD=     $(LDADD_RL)

gras/datadesc/datadesc_structs.c: gras/datadesc/mk_datadesc_structs.pl; \
	perl $(top_srcdir)/teshsuite/gras/datadesc/mk_datadesc_structs.pl > $@

  ## Ensures the gras_msg_handle semantic ##
  ##########################################
EXTRA_DIST += gras/msg_handle/msg_handle.xml \
              gras/msg_handle/test_rl \
              gras/msg_handle/test_sg_32 gras/msg_handle/test_sg_64
noinst_PROGRAMS+=\
  gras/msg_handle/msg_handle_client \
  gras/msg_handle/msg_handle_server \
  gras/msg_handle/msg_handle_simulator

gras_msg_handle_msg_handle_simulator_SOURCES= gras/msg_handle/msg_handle.c gras/msg_handle/_msg_handle_simulator.c
gras_msg_handle_msg_handle_client_SOURCES=    gras/msg_handle/msg_handle.c gras/msg_handle/_msg_handle_client.c
gras_msg_handle_msg_handle_server_SOURCES=    gras/msg_handle/msg_handle.c gras/msg_handle/_msg_handle_server.c

gras_msg_handle_msg_handle_simulator_LDADD=   $(LDADD_SG)
gras_msg_handle_msg_handle_client_LDADD=      $(LDADD_RL)
gras_msg_handle_msg_handle_server_LDADD=      $(LDADD_RL)

gras/msg_handle/_msg_handle_simulator.c gras/msg_handle/_msg_handle_client.c gras/msg_handle/_msg_handle_server.c : gras/msg_handle/msg_handle.c gras/msg_handle/msg_handle.xml $(top_builddir)/tools/gras/gras_stub_generator@EXEEXT@; \
	mkdir -p gras/msg_handle; \
	  cd gras/msg_handle;\
	  $(abs_top_builddir)/tools/gras/gras_stub_generator@EXEEXT@ msg_handle $(abs_top_srcdir)/teshsuite/gras/msg_handle/msg_handle.xml


if GRAS_ARCH_32_BITS
  TESTS += gras/msg_handle/test_rl gras/msg_handle/test_sg_32
else
  TESTS += gras/msg_handle/test_rl gras/msg_handle/test_sg_64
endif

  ## Make sure it's ok to have empty main functions ##
  ####################################################
EXTRA_DIST += gras/empty_main/empty_main.xml \
              gras/empty_main/test_rl gras/empty_main/test_sg
noinst_PROGRAMS+=\
  gras/empty_main/empty_main_function \
  gras/empty_main/empty_main_simulator

gras_empty_main_empty_main_simulator_SOURCES= gras/empty_main/empty_main.c gras/empty_main/_empty_main_simulator.c
gras_empty_main_empty_main_function_SOURCES=  gras/empty_main/empty_main.c gras/empty_main/_empty_main_function.c

gras_empty_main_empty_main_simulator_LDADD=   $(LDADD_SG)
gras_empty_main_empty_main_function_LDADD=    $(LDADD_RL)

gras/empty_main/_empty_main_simulator.c gras/empty_main/_empty_main_function.c : gras/empty_main/empty_main.c gras/empty_main/empty_main.xml $(top_builddir)/tools/gras/gras_stub_generator@EXEEXT@ ; \
	mkdir -p gras/empty_main; \
	  cd gras/empty_main;\
	  $(abs_top_builddir)/tools/gras/gras_stub_generator@EXEEXT@ empty_main $(abs_top_srcdir)/teshsuite/gras/empty_main/empty_main.xml

TESTS += gras/empty_main/test_rl gras/empty_main/test_sg

  ## Make sure it's ok to have very small sleeps (not meant for real life) ##
  ###########################################################################
EXTRA_DIST += gras/small_sleep/small_sleep.xml \
              gras/small_sleep/test_sg_32 gras/small_sleep/test_sg_64
noinst_PROGRAMS+=\
  gras/small_sleep/small_sleep_function \
  gras/small_sleep/small_sleep_simulator

gras_small_sleep_small_sleep_simulator_SOURCES= gras/small_sleep/small_sleep.c gras/small_sleep/_small_sleep_simulator.c
gras_small_sleep_small_sleep_function_SOURCES=  gras/small_sleep/small_sleep.c gras/small_sleep/_small_sleep_function.c

gras_small_sleep_small_sleep_simulator_LDADD=   $(LDADD_SG)
gras_small_sleep_small_sleep_function_LDADD=    $(LDADD_RL)

gras/small_sleep/_small_sleep_simulator.c gras/small_sleep/_small_sleep_function.c : gras/small_sleep/small_sleep.c gras/small_sleep/small_sleep.xml $(top_builddir)/tools/gras/gras_stub_generator@EXEEXT@; \
	mkdir -p gras/small_sleep; \
	  cd gras/small_sleep;\
	  $(abs_top_builddir)/tools/gras/gras_stub_generator@EXEEXT@ small_sleep $(abs_top_srcdir)/teshsuite/gras/small_sleep/small_sleep.xml

if GRAS_ARCH_32_BITS
  TESTS += gras/small_sleep/test_sg_32
else
  TESTS += gras/small_sleep/test_sg_64
endif

#############
# MSG tests #
#############

noinst_PROGRAMS += msg/get_sender
EXTRA_DIST      += msg/get_sender.xml \
		   msg/get_sender.tesh
msg_get_sender_SOURCES = msg/get_sender.c
msg_get_sender_LDADD   = $(LDADD_SG)
TESTS += msg/get_sender.tesh

###################################
# network model test via SimDag API
###################################

# misc tests

noinst_PROGRAMS += simdag/network/test_reinit_costs
EXTRA_DIST      += simdag/network/platform_2p_1sl.xml
simdag_network_test_reinit_costs_SOURCES = simdag/network/test_reinit_costs.c
simdag_network_test_reinit_costs_LDADD   =   $(LDADD_SG)
TESTS += \
  simdag/network/test_reinit_costs.tesh

noinst_PROGRAMS += simdag/platforms/basic_parsing_test
simdag_platforms_basic_parsing_test_SOURCES = simdag/platforms/basic_parsing_test.c
simdag_platforms_basic_parsing_test_LDADD = $(LDADD_SG)
TESTS += simdag/platforms/basic_parsing_test.tesh
XFAIL_TESTS += simdag/platforms/basic_parsing_test_failing.tesh

noinst_PROGRAMS += simdag/platforms/flatifier
simdag_platforms_flatifier_SOURCES = simdag/platforms/flatifier.c
simdag_platforms_flatifier_LDADD = $(LDADD_SG)
TESTS += simdag/platforms/flatifier.tesh

EXTRA_DIST      += simdag/platforms/one_cluster_foreach.xml \
                   simdag/platforms/one_cluster_one_host_random.xml \
                   simdag/platforms/one_cluster_one_host.xml \
                   simdag/platforms/one_cluster_properties_foreach.xml \
                   simdag/platforms/one_cluster_properties_override.xml \
                   simdag/platforms/one_cluster.xml \
                   simdag/platforms/one_host_availability.xml \
                   simdag/platforms/one_host_properties.xml \
                   simdag/platforms/one_host_state_file.xml \
                   simdag/platforms/one_host_state_off.xml \
                   simdag/platforms/one_host_state_on.xml \
                   simdag/platforms/one_host_trace_file.xml \
                   simdag/platforms/one_host_trace_inside.xml \
                   simdag/platforms/one_host.xml \
                   simdag/platforms/one_link_availability.xml \
                   simdag/platforms/one_link_fatpipe.xml \
                   simdag/platforms/one_link_shared.xml \
                   simdag/platforms/one_link_state_file.xml \
                   simdag/platforms/route_override.xml \
                   simdag/platforms/three_hosts_non_symmetric_route.xml \
                   simdag/platforms/two_clusters_symmetric.xml \
                   simdag/platforms/two_clusters.xml \
                   simdag/platforms/two_hosts_multi_hop.xml \
                   simdag/platforms/two_hosts_one_link.xml \
		   simdag/platforms/bob.fail \
		   simdag/platforms/bob.trace \
		   simdag/platforms/link1.bw \
		   simdag/platforms/link1.fail \
		   simdag/platforms/link1.lat 

# from L.C. Canon
noinst_PROGRAMS += simdag/basic0 \
                   simdag/basic1 \
                   simdag/basic2 \
                   simdag/basic3 \
                   simdag/basic4 \
                   simdag/basic5 \
                   simdag/basic6

EXTRA_DIST      += simdag/basic_platform.xml

simdag_basic0_SOURCES = simdag/basic0.c
simdag_basic0_LDADD = $(LDADD_SG)
simdag_basic1_SOURCES = simdag/basic1.c
simdag_basic1_LDADD = $(LDADD_SG)
simdag_basic2_SOURCES = simdag/basic2.c
simdag_basic2_LDADD = $(LDADD_SG)
simdag_basic3_SOURCES = simdag/basic3.c
simdag_basic3_LDADD = $(LDADD_SG)
simdag_basic4_SOURCES = simdag/basic4.c
simdag_basic4_LDADD = $(LDADD_SG)
simdag_basic5_SOURCES = simdag/basic5.c
simdag_basic5_LDADD = $(LDADD_SG)
simdag_basic6_SOURCES = simdag/basic6.c
simdag_basic6_LDADD = $(LDADD_SG)

TESTS += \
  simdag/basic0.tesh  \
  simdag/basic1.tesh \
  simdag/basic2.tesh \
  simdag/basic3.tesh \
  simdag/basic4.tesh \
  simdag/basic5.tesh \
  simdag/basic6.tesh

# p2p

noinst_PROGRAMS += simdag/network/p2p/test_latency1 \
                   simdag/network/p2p/test_latency2 \
                   simdag/network/p2p/test_latency3 \
                   simdag/network/p2p/test_latency_bound

EXTRA_DIST      += simdag/network/p2p/platform_2p_1sl.xml \
                   simdag/network/p2p/platform_2p_1fl.xml \
                   simdag/network/p2p/platform_2p_1switch.xml \
                   simdag/network/p2p/platform_2p_1bb.xml

simdag_network_p2p_test_latency1_SOURCES = simdag/network/p2p/test_latency1.c
simdag_network_p2p_test_latency1_LDADD   = $(LDADD_SG)
simdag_network_p2p_test_latency2_SOURCES = simdag/network/p2p/test_latency2.c
simdag_network_p2p_test_latency2_LDADD   = $(LDADD_SG)
simdag_network_p2p_test_latency3_SOURCES = simdag/network/p2p/test_latency3.c
simdag_network_p2p_test_latency3_LDADD   = $(LDADD_SG)
simdag_network_p2p_test_latency_bound_SOURCES = simdag/network/p2p/test_latency_bound.c
simdag_network_p2p_test_latency_bound_LDADD   = $(LDADD_SG)

TESTS += \
  simdag/network/p2p/test_latency1.tesh \
  simdag/network/p2p/test_latency2.tesh \
  simdag/network/p2p/test_latency3.tesh \
  simdag/network/p2p/test_latency_bound.tesh

# m x n tests

noinst_PROGRAMS += simdag/network/mxn/test_intra_all2all \
                   simdag/network/mxn/test_intra_independent_comm \
                   simdag/network/mxn/test_intra_scatter

EXTRA_DIST      += simdag/network/mxn/platform_4p_1switch.xml 

simdag_network_mxn_test_intra_all2all_SOURCES = simdag/network/mxn/test_intra_all2all.c
simdag_network_mxn_test_intra_all2all_LDADD   = $(LDADD_SG)
simdag_network_mxn_test_intra_independent_comm_SOURCES = simdag/network/mxn/test_intra_independent_comm.c
simdag_network_mxn_test_intra_independent_comm_LDADD   = $(LDADD_SG)
simdag_network_mxn_test_intra_scatter_SOURCES = simdag/network/mxn/test_intra_scatter.c
simdag_network_mxn_test_intra_scatter_LDADD   = $(LDADD_SG)

TESTS += \
  simdag/network/mxn/test_intra_all2all.tesh \
  simdag/network/mxn/test_intra_independent_comm.tesh \
  simdag/network/mxn/test_intra_scatter.tesh 

# testing parallel tasks

noinst_PROGRAMS += simdag/partask/test_comp_only_seq \
                   simdag/partask/test_comp_only_par 

EXTRA_DIST      += simdag/partask/platform_2p_1sl_hetero.xml \
                   simdag/partask/platform_2p_1sl.xml

simdag_partask_test_comp_only_seq_SOURCES = simdag/partask/test_comp_only_seq.c
simdag_partask_test_comp_only_seq_LDADD   = $(LDADD_SG)
simdag_partask_test_comp_only_par_SOURCES = simdag/partask/test_comp_only_par.c
simdag_partask_test_comp_only_par_LDADD   = $(LDADD_SG)

TESTS += \
  simdag/partask/test_comp_only_seq.tesh \
  simdag/partask/test_comp_only_par.tesh

EXTRA_DIST+=$(TESTS)
include $(top_srcdir)/acmacro/dist-files.mk
