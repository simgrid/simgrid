INCLUDES= -I$(top_srcdir)/include
AM_CFLAGS=-g
EXTRA_DIST= msg_deployment.xml msg_platform.xml 

check_PROGRAMS=msg_test

msg_test_SOURCES= messages.h msg_test.c
msg_test_LDADD=	$(top_builddir)/src/libsimgrid.la

