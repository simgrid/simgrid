# Copyright (c) 2004-2007. The SimGrid team. All right reserved.

# This file is part of the SimGrid project. This is free software:
# You can redistribute and/or modify it under the terms of the
# GNU LGPL (v2.1) licence.


INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src -I$(top_srcdir)/src/include
AM_CFLAGS = -g

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
  gras/datadesc/datadesc_r_big32_8_4.tesh  \
  gras/datadesc/datadesc_r_big32_2.tesh

noinst_PROGRAMS += gras/datadesc/datadesc_usage   
gras_datadesc_datadesc_usage_SOURCES=   gras/datadesc/datadesc_usage.c gras/datadesc/datadesc_structs.c
gras_datadesc_datadesc_usage_LDADD=     $(LDADD_RL)

gras/datadesc/datadesc_structs.c: gras/datadesc/mk_datadesc_structs.pl
	perl $(top_srcdir)/teshsuite/gras/datadesc/mk_datadesc_structs.pl > $@

  ## Ensures the gras_msg_handle semantic ##
  ##########################################
EXTRA_DIST += gras/msg_handle/msg_handle.xml
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

gras/msg_handle/_msg_handle_simulator.c gras/msg_handle/_msg_handle_client.c gras/msg_handle/_msg_handle_server.c : gras/msg_handle/msg_handle.c gras/msg_handle/msg_handle.xml $(top_builddir)/tools/gras/gras_stub_generator@EXEEXT@
	mkdir -p gras/msg_handle; \
	  cd gras/msg_handle;\
	  ../../$(top_builddir)/tools/gras/gras_stub_generator@EXEEXT@ msg_handle ../../$(top_srcdir)/teshsuite/gras/msg_handle/msg_handle.xml


if GRAS_ARCH_32_BITS
  TESTS += gras/msg_handle/test_rl gras/msg_handle/test_sg_32
else
  TESTS += gras/msg_handle/test_rl gras/msg_handle/test_sg_64
endif


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

include $(top_srcdir)/acmacro/dist-files.mk
