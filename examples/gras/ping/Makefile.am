INCLUDES= -I$(top_srcdir)/include

EXTRA_DIST=ping.xml ping.h
include $(top_srcdir)/examples/gras/tests.mk

# AUTOMAKE variable definition
noinst_PROGRAMS=ping_client ping_server ping_simulator

ping_simulator_SOURCES=	_ping_simulator.c ping_server.c ping_client.c ping_common.c
ping_simulator_LDADD=	$(top_builddir)/src/libsimgrid.la

ping_client_SOURCES=	_ping_client.c                  ping_client.c ping_common.c
ping_client_LDADD=	$(top_builddir)/src/libgras.la

ping_server_SOURCES=	_ping_server.c    ping_server.c               ping_common.c
ping_server_LDADD=	$(top_builddir)/src/libgras.la

# Take care of generatated sources
NAME=ping
PROCESSES= client server
include $(top_srcdir)/examples/temps-gras-stub.mk

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
