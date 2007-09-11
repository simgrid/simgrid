INCLUDES  = -I$(top_srcdir)/include
AM_CFLAGS = -g


TESTS_ENVIRONMENT = $(top_builddir)/tools/tesh/tesh


TESTS = xbt/parallel_log_crashtest.tesh 

noinst_PROGRAMS =xbt/parallel_log_crashtest

# Try parallel logs 
xbt_parallel_log_crashtest_LDADD = $(top_builddir)/src/libgras.la

include $(top_srcdir)/acmacro/dist-files.mk
