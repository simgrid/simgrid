# Copyright (c) 2004-2007. The SimGrid team. All right reserved.

# This file is part of the SimGrid project. This is free software:
# You can redistribute and/or modify it under the terms of the
# GNU LGPL (v2.1) licence.


INCLUDES= -I$(top_srcdir)/include
EXTRA_DIST=rpc.xml
include $(top_srcdir)/examples/gras/tests.mk

# AUTOMAKE variable definition
noinst_PROGRAMS=rpc_client rpc_forwarder rpc_server rpc_simulator

rpc_simulator_SOURCES=	_rpc_simulator.c rpc.c
rpc_simulator_LDADD=	$(top_builddir)/src/libsimgrid.la

rpc_client_SOURCES=	_rpc_client.c rpc.c
rpc_client_LDADD=	$(top_builddir)/src/libgras.la

rpc_forwarder_SOURCES=	_rpc_forwarder.c rpc.c
rpc_forwarder_LDADD=	$(top_builddir)/src/libgras.la

rpc_server_SOURCES=	_rpc_server.c rpc.c
rpc_server_LDADD=	$(top_builddir)/src/libgras.la

# Take care of generatated sources
NAME=rpc
PROCESSES= client forwarder server
include $(top_srcdir)/examples/temps-gras-stub.mk

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
