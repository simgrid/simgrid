INCLUDES= -I$(top_srcdir)/include
AM_CFLAGS=-g
EXTRA_DIST= msg_deployment.xml msg_platform.xml small_platform.xml small_deployment.xml msg_deployment_suspend.xml

CLEANFILES=msg_test.trace

TESTS=run_msg_test
check_PROGRAMS=msg_test msg_test_surfxml_bypassed msg_test_suspend ping_pong

msg_test_SOURCES= msg_test.c
msg_test_LDADD=	$(top_builddir)/src/libsimgrid.la

msg_test_surfxml_bypassed_SOURCES = msg_test_surfxml_bypassed.c
msg_test_surfxml_bypassed_LDADD = $(top_builddir)/src/libsimgrid.la

msg_test_suspend_SOURCES= msg_test_suspend.c
msg_test_suspend_LDADD=	$(top_builddir)/src/libsimgrid.la

ping_pong_SOURCES= ping_pong.c
ping_pong_LDADD= $(top_builddir)/src/libsimgrid.la

include $(top_srcdir)/acmacro/dist-files.mk
