# Copyright (c) 2004-2007. The SimGrid team. All right reserved.

# This file is part of the SimGrid project. This is free software:
# You can redistribute and/or modify it under the terms of the
# GNU LGPL (v2.1) licence.


INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src -I$(top_srcdir)/src/include
AM_CFLAGS=-g

CLEANFILES = *~ test a.out *.o testgraph.xml testgraph.dot
DISTCLEANFILES =  gras/.libs/* xbt/.libs/* surf/.libs/* msg/.libs/* simdag/.libs/*

EXTRA_DIST=run_tests.in \
  xbt/graph.xml \
  surf/trace_A.txt surf/trace_B.txt surf/trace_A_failure.txt surf/platform.xml\
  simdag/availability_tremblay.txt simdag/small_platform_variable.xml
  
# Test stuff

xbt_tests =                                                      \
	xbt/log_usage                                            \
	xbt/heap_bench                                           \
	xbt/graphxml_usage                                       

SG_tests =                                              \
	surf/maxmin_bench surf/lmm_usage \
	surf/trace_usage surf/surf_usage surf/surf_usage2

simdag_tests = simdag/sd_test

check_PROGRAMS = $(xbt_tests) $(SG_tests) $(simdag_tests)
check_SCRIPTS = run_tests 
TESTS=run_tests

test: $(noinst_PROGRAMS) $(noinst_SCRIPTS)
	./run_tests
valgrind: $(noinst_PROGRAMS) $(noinst_SCRIPTS)
	./run_tests valgrind

# LDADD_UTILS is still a separate var to distinguish core tester from RL tester
LDADD_SG=$(abs_top_builddir)/src/libsimgrid.la 
LDADD_RL=$(abs_top_builddir)/src/libgras.la   

xbt_log_usage_LDADD=          $(LDADD_RL)

xbt_heap_bench_LDADD=         $(LDADD_RL)

xbt_graphxml_usage_LDADD=     $(LDADD_SG)


surf_maxmin_bench_LDADD=      $(LDADD_SG)
surf_lmm_usage_LDADD=         $(LDADD_SG)
surf_trace_usage_LDADD=       $(LDADD_SG)
surf_surf_usage_LDADD=        $(LDADD_SG)
surf_surf_usage2_LDADD=       $(LDADD_SG)

simdag_sd_test_LDADD=       $(LDADD_SG)

include $(top_srcdir)/acmacro/dist-files.mk
