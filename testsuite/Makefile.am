INCLUDES = -I$(top_srcdir)/src/include
AM_CFLAGS=-g -Wno-unused-function 
AM_LDFLAGS=-g

CLEANFILES = *~ test a.out *.o
DISTCLEANFILES=Makefile.in *~

# Test stuff

noinst_PROGRAMS =                             \
	dynar_int dynar_double dynar_string   \
	dict_usage dict_crash                 \
	log_usage                             \
	config_usage                          
	\
	gs_example_send gs_example_receive
	
# multidict_crash 


noinst_SCRIPTS = run_tests gs_example

dynar_int_LDADD=          $(top_srcdir)/src/base/libgrasutils.a
dynar_double_LDADD=       $(top_srcdir)/src/base/libgrasutils.a
dynar_string_LDADD=       $(top_srcdir)/src/base/libgrasutils.a

dict_usage_LDADD=         $(top_srcdir)/src/base/libgrasutils.a
dict_crash_LDADD=         $(top_srcdir)/src/base/libgrasutils.a
multidict_crash_LDADD=    $(top_srcdir)/src/base/libgrasutils.a

config_usage_LDADD=       $(top_srcdir)/src/base/libgrasutils.a

log_usage_LDADD=           $(top_srcdir)/src/base/libgrasutils.a

gs_example_send_LDADD=    $(top_srcdir)/src/base/libgrasutils.a
gs_example_receive_LDADD= $(top_srcdir)/src/base/libgrasutils.a

TESTS=run_tests

test: $(noinst_PROGRAMS)
	./run_tests
