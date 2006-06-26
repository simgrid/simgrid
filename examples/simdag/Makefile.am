INCLUDES= -I$(top_srcdir)/include
AM_CFLAGS=-g

#TESTS=run_msg_test
check_PROGRAMS=sd_test

sd_test_SOURCES= sd_test.c
sd_test_LDADD=	$(top_builddir)/src/libsimgrid.la

include $(top_srcdir)/acmacro/dist-files.mk
