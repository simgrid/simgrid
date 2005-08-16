INCLUDES= -I$(top_srcdir)/include
AM_CFLAGS=-g
EXTRA_DIST= msg_deployment.xml msg_platform.xml small_platform.xml small_deployment.xml

CLEANFILES=msg_test.trace

TESTS=run_msg_test
check_PROGRAMS=msg_test msg_test_surfxml_bypassed

msg_test_SOURCES= msg_test.c
msg_test_LDADD=	$(top_builddir)/src/libsimgrid.la

msg_test_surfxml_bypassed_SOURCES = msg_test_surfxml_bypassed.c
msg_test_surfxml_bypassed_LDADD = $(top_builddir)/src/libsimgrid.la

include $(top_srcdir)/acmacro/dist-files.mk
