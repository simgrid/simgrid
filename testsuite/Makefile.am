INCLUDES = -I$(top_srcdir)/src/include
AM_CFLAGS=-g -Wno-unused-function 
AM_LDFLAGS=-g

CLEANFILES = *~ test a.out *.o
DISTCLEANFILES=Makefile.in *~

# Test stuff

TESTS_PROGS=                                  \
	dynar_int dynar_double dynar_string   \
	dict_usage dict_crash                 \
	test_log                              \
	config_usage                          

# multidict_crash 


noinst_PROGRAMS = $(TESTS_PROGS) gs_example_send gs_example_receive
noinst_SCRIPTS = gs_example

dynar_int_LDADD=          $(top_srcdir)/src/base/libgrasutils.a
dynar_double_LDADD=       $(top_srcdir)/src/base/libgrasutils.a
dynar_string_LDADD=       $(top_srcdir)/src/base/libgrasutils.a

dict_usage_LDADD=         $(top_srcdir)/src/base/libgrasutils.a
dict_crash_LDADD=         $(top_srcdir)/src/base/libgrasutils.a
multidict_crash_LDADD=    $(top_srcdir)/src/base/libgrasutils.a

config_usage_LDADD=       $(top_srcdir)/src/base/libgrasutils.a

test_log_LDADD=           $(top_srcdir)/src/base/libgrasutils.a

gs_example_send_LDADD=    $(top_srcdir)/src/base/libgrasutils.a
gs_example_receive_LDADD= $(top_srcdir)/src/base/libgrasutils.a

TESTS=$(TESTS_PROGS) #gs_example
