INCLUDES= -I$(top_srcdir)/include
AM_CFLAGS=-g

EXTRA_DIST=$(NAME).xml $(TESTS)
include $(top_srcdir)/examples/gras/tests.mk

# AUTOMAKE variable definition
noinst_PROGRAMS=timer_client timer_simulator

timer_simulator_SOURCES=	_timer_simulator.c timer.c
timer_simulator_LDADD=	$(top_builddir)/src/libsimgrid.la

timer_client_SOURCES=	_timer_client.c timer.c
timer_client_LDADD=	$(top_builddir)/src/libgras.la

# Take care of generatated sources
NAME=timer
PROCESSES= client
include $(top_srcdir)/examples/temps-gras-stub.mk

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
