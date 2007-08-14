INCLUDES = -I$(top_srcdir)/include
AM_CFLAGS = -g
EXTRA_DIST = 2clusters.xml

CLEANFILES = sd_test.trace

TESTS_ENVIRONMENT = $(top_builddir)/tools/tesh/tesh

TESTS = test_simdag.tesh test_simdag2.tesh 
check_PROGRAMS = sd_test sd_test2

bin_PROGRAMS = 

sd_test_SOURCES = sd_test.c
sd_test_LDADD =	$(top_builddir)/src/libsimgrid.la

sd_test2_SOURCES = sd_test2.c
sd_test2_LDADD = $(top_builddir)/src/libsimgrid.la

include $(top_srcdir)/acmacro/dist-files.mk
