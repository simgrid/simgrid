INCLUDES= -I$(top_srcdir)/include
AM_CFLAGS=-g
EXTRA_DIST= small_platform.xml

CLEANFILES=sd_test.trace

TESTS=test_simdag
check_PROGRAMS=sd_test small_platform.xml

sd_test_SOURCES= sd_test.c
sd_test_LDADD=	$(top_builddir)/src/libsimgrid.la

include $(top_srcdir)/acmacro/dist-files.mk
