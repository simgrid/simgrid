INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src
AM_CFLAGS=-g

CLEANFILES = *~ test a.out *.o datadesc_usage.out 
DISTCLEANFILES =  gras/.libs/* gros/.libs/*
MAINTAINERCLEANFILES=Makefile.in
EXTRA_DIST=run_tests.in \
  gras/datadesc.little32 gras/datadesc.little64 gras/datadesc.big32 gras/datadesc.big64 \
  gras/mk_datadesc_structs.pl

# Test stuff

gros_tests =                                                      \
	gros/dynar_int    gros/dynar_double   gros/dynar_string   \
	gros/dict_usage   gros/dict_crash                         \
	gros/set_usage                                            \
	gros/log_usage                                            \
	gros/config_usage
# gros/multidict_crash 

RL_tests =                                              \
	gras/trp_tcp_client   gras/trp_tcp_server       \
	gras/trp_file_client  gras/trp_file_server      \
	gras/datadesc_usage

SG_tests =

check_PROGRAMS = $(gros_tests) $(RL_tests) $(SG_tests)
check_SCRIPTS = run_tests gras/trp_tcp_usage
TESTS=run_tests

test: $(noinst_PROGRAMS) $(noinst_SCRIPTS)
	./run_tests
valgrind: $(noinst_PROGRAMS) $(noinst_SCRIPTS)
	./run_tests valgrind

# LDADD_UTILS is still a separate var to distinguish core tester from RL tester
LDADD_UTILS=$(top_builddir)/src/libgrasrl.la 
LDADD_RL=   $(top_builddir)/src/libgrasrl.la   

gros_log_usage_LDADD=          $(LDADD_UTILS)

gros_dynar_int_LDADD=          $(LDADD_UTILS)
gros_dynar_double_LDADD=       $(LDADD_UTILS)
gros_dynar_string_LDADD=       $(LDADD_UTILS)

gros_dict_usage_LDADD=         $(LDADD_UTILS)
gros_dict_crash_LDADD=         $(LDADD_UTILS)
#gros_multidict_crash_LDADD=    $(LDADD_UTILS)

gros_set_usage_LDADD=          $(LDADD_UTILS)

gros_config_usage_LDADD=       $(LDADD_UTILS)

gras_trp_tcp_client_LDADD=     $(LDADD_RL)
gras_trp_tcp_server_LDADD=     $(LDADD_RL)
gras_trp_file_client_LDADD=    $(LDADD_RL)
gras_trp_file_server_LDADD=    $(LDADD_RL)

gras_datadesc_usage_SOURCES= gras/datadesc_usage.c gras/datadesc_structs.c
gras_datadesc_usage_LDADD=     $(LDADD_RL)

gras/datadesc_structs.c: gras/mk_datadesc_structs.pl
	cd gras ; perl mk_datadesc_structs.pl > datadesc_structs.c
