INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src -I$(top_srcdir)/src/include
AM_CFLAGS=-g

CLEANFILES = *~ test a.out *.o gras/datadesc_usage.out datadesc_usage.out\
  testgraph.xml testgraph.dot
DISTCLEANFILES =  gras/.libs/* xbt/.libs/* surf/.libs/* msg/.libs/* simdag/.libs/*

EXTRA_DIST=run_tests.in \
  gras/datadesc.little32 gras/datadesc.little32_4 \
  gras/datadesc.little64\
  gras/datadesc.big32 gras/datadesc.big32_8_4 gras/datadesc.big32_2 \
  gras/mk_datadesc_structs.pl \
  xbt/graph.xml \
  surf/trace_A.txt surf/trace_B.txt surf/trace_A_failure.txt surf/platform.xml\
  simdag/availability_tremblay.txt simdag/small_platform_variable.xml
  
# Data sets still to regenerate:
# gras/datadesc.big64 
# gras/datadesc.big32_4 


# Test stuff

xbt_tests =                                                      \
	xbt/log_usage                                            \
	xbt/heap_bench                                           \
	xbt/graphxml_usage                                       \
	xbt/context_usage                                        

RL_tests =                                              \
	gras/trp_tcp_client   gras/trp_tcp_server       \
	gras/trp_file_client  gras/trp_file_server      \
	gras/datadesc_usage                             

SG_tests =                                              \
	surf/maxmin_usage surf/maxmin_bench surf/simeng_usage \
	surf/trace_usage surf/surf_usage surf/surf_usage2
#if HAVE_SDP
#  SG_tests+=surf/sdp_usage
#endif

simdag_tests = simdag/sd_test

check_PROGRAMS = $(xbt_tests) $(RL_tests) $(SG_tests) $(simdag_tests)
check_SCRIPTS = run_tests gras/trp_tcp_usage
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

xbt_context_usage_LDADD=      $(LDADD_SG)

surf_maxmin_usage_LDADD=      $(LDADD_SG)
surf_maxmin_bench_LDADD=      $(LDADD_SG)
surf_simeng_usage_LDADD=      $(LDADD_SG)
surf_trace_usage_LDADD=       $(LDADD_SG)
surf_surf_usage_LDADD=        $(LDADD_SG)
surf_surf_usage2_LDADD=       $(LDADD_SG)
#if HAVE_SDP
#surf_sdp_usage_LDADD=         $(LDADD_SG)
#surf_sdp_bench_LDADD=         $(LDADD_SG)
#endif

gras_trp_tcp_client_LDADD=     $(LDADD_RL)
gras_trp_tcp_server_LDADD=     $(LDADD_RL)
gras_trp_file_client_LDADD=    $(LDADD_RL)
gras_trp_file_server_LDADD=    $(LDADD_RL)

gras_datadesc_usage_SOURCES=   gras/datadesc_usage.c gras/datadesc_structs.c
gras_datadesc_usage_LDADD=     $(LDADD_RL)

gras/datadesc_structs.c: gras/mk_datadesc_structs.pl
	cd gras ; perl ../$(top_srcdir)/testsuite/gras/mk_datadesc_structs.pl > datadesc_structs.c

simdag_sd_test_LDADD=       $(LDADD_SG)

include $(top_srcdir)/acmacro/dist-files.mk
