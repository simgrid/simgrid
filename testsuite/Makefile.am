INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src -I$(top_srcdir)/src/include
AM_CFLAGS=-g

CLEANFILES = *~ test a.out *.o gras/datadesc_usage.out datadesc_usage.out
DISTCLEANFILES =  gras/.libs/* xbt/.libs/* surf/.libs/* msg/.libs/*
MAINTAINERCLEANFILES=Makefile.in
EXTRA_DIST=run_tests.in \
  gras/datadesc.little32 gras/datadesc.little32_4 \
  gras/datadesc.little64\
  gras/datadesc.big32 gras/datadesc.big32_8_4 \
  gras/mk_datadesc_structs.pl \
  surf/trace_A.txt surf/trace_B.txt surf/trace_A_failure.txt surf/platform.xml \
  msg/messages.h

# Data sets still to regenerate:
# gras/datadesc.big64 
# gras/datadesc.big32_4 gras/datadesc.big32_2 


# Test stuff

xbt_tests =                                                      \
	xbt/dynar_int    xbt/dynar_double   xbt/dynar_string     \
	xbt/dict_usage   xbt/dict_crash                          \
	xbt/set_usage                                            \
	xbt/log_usage                                            \
	xbt/heap_bench                                           \
	xbt/swag_usage                                           \
	xbt/context_usage                                        \
	xbt/config_usage
# xbt/multidict_crash 

RL_tests =                                              \
	gras/trp_tcp_client   gras/trp_tcp_server       \
	gras/trp_file_client  gras/trp_file_server      \
	gras/datadesc_usage

SG_tests =                                              \
	surf/maxmin_usage surf/maxmin_bench \
	surf/trace_usage surf/surf_usage surf/surf_usage2 \
	msg/msg_test

check_PROGRAMS = $(xbt_tests) $(RL_tests) $(SG_tests)
check_SCRIPTS = run_tests gras/trp_tcp_usage
TESTS=run_tests

test: $(noinst_PROGRAMS) $(noinst_SCRIPTS)
	./run_tests
valgrind: $(noinst_PROGRAMS) $(noinst_SCRIPTS)
	./run_tests valgrind

# LDADD_UTILS is still a separate var to distinguish core tester from RL tester
LDADD_UTILS=$(top_builddir)/src/libgras.la 
LDADD_RL=   $(top_builddir)/src/libgras.la   

xbt_log_usage_LDADD=          $(LDADD_UTILS)

xbt_dynar_int_LDADD=          $(LDADD_UTILS)
xbt_dynar_double_LDADD=       $(LDADD_UTILS)
xbt_dynar_string_LDADD=       $(LDADD_UTILS)

xbt_dict_usage_LDADD=         $(LDADD_UTILS)
xbt_dict_crash_LDADD=         $(LDADD_UTILS)
#xbt_multidict_crash_LDADD=    $(LDADD_UTILS)

xbt_set_usage_LDADD=          $(LDADD_UTILS)
xbt_swag_usage_LDADD=         $(LDADD_UTILS)

xbt_config_usage_LDADD=       $(LDADD_UTILS)

xbt_heap_bench_LDADD=         $(LDADD_UTILS)
xbt_context_usage_LDADD=      $(LDADD_UTILS)

surf_maxmin_usage_LDADD=      $(LDADD_UTILS)
surf_maxmin_bench_LDADD=      $(LDADD_UTILS)
surf_trace_usage_LDADD=       $(LDADD_UTILS)
surf_surf_usage_LDADD=        $(LDADD_UTILS)
surf_surf_usage2_LDADD=       $(LDADD_UTILS)

msg_msg_test_LDADD=           $(LDADD_UTILS)

gras_trp_tcp_client_LDADD=     $(LDADD_RL)
gras_trp_tcp_server_LDADD=     $(LDADD_RL)
gras_trp_file_client_LDADD=    $(LDADD_RL)
gras_trp_file_server_LDADD=    $(LDADD_RL)

gras_datadesc_usage_SOURCES= gras/datadesc_usage.c gras/datadesc_structs.c
gras_datadesc_usage_LDADD=     $(LDADD_RL)

gras/datadesc_structs.c: gras/mk_datadesc_structs.pl
	cd gras ; perl mk_datadesc_structs.pl > datadesc_structs.c
