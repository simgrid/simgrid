INCLUDES = -I$(top_srcdir)/src/include
AM_CFLAGS=-g -Wno-unused-function 

CLEANFILES = *~ test a.out *.o
DISTCLEANFILES=Makefile.in *~
EXTRA_DIST=run_tests.in

# Test stuff

core_tests =                                  \
	dynar_int dynar_double dynar_string   \
	dict_usage dict_crash                 \
	set_usage                             \
	log_usage                             \
	config_usage
# multidict_crash 

RL_tests =                                    \
	trp_tcp_client   trp_tcp_server       \
	trp_file_client  trp_file_server      \
	datadesc_usage

SG_tests =

noinst_PROGRAMS = $(core_tests) $(RL_tests) $(SG_tests)
noinst_SCRIPTS = run_tests trp_tcp_usage
TESTS=run_tests

test: $(noinst_PROGRAMS) $(noinst_SCRIPTS)
	./run_tests
valgrind: $(noinst_PROGRAMS) $(noinst_SCRIPTS)
	./run_tests valgrind

# LDADD_UTILS is still a separate var to distinguish core tester from RL tester
LDADD_UTILS=$(top_srcdir)/src/base/libgrasrl.a 
LDADD_RL=   $(top_srcdir)/src/base/libgrasrl.a   

log_usage_LDADD=          $(LDADD_UTILS)

dynar_int_LDADD=          $(LDADD_UTILS)
dynar_double_LDADD=       $(LDADD_UTILS)
dynar_string_LDADD=       $(LDADD_UTILS)

dict_usage_LDADD=         $(LDADD_UTILS)
dict_crash_LDADD=         $(LDADD_UTILS)
#multidict_crash_LDADD=    $(LDADD_UTILS)

set_usage_LDADD=          $(LDADD_UTILS)

config_usage_LDADD=       $(LDADD_UTILS)

trp_tcp_client_LDADD=     $(LDADD_RL)
trp_tcp_server_LDADD=     $(LDADD_RL)
trp_file_client_LDADD=    $(LDADD_RL)
trp_file_server_LDADD=    $(LDADD_RL)

datadesc_usage_LDADD=     $(LDADD_RL)

