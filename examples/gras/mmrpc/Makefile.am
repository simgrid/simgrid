INCLUDES= -I$(top_srcdir)/include
TESTS= test_rl test_sg
EXTRA_DIST=mmrpc.xml mmrpc.h $(TESTS)

# AUTOMAKE variable definition
noinst_PROGRAMS=mmrpc_client mmrpc_server mmrpc_simulator

mmrpc_simulator_SOURCES=_mmrpc_simulator.c mmrpc_common.c mmrpc_client.c mmrpc_server.c
mmrpc_simulator_LDADD=	$(top_builddir)/src/libsimgrid.la

mmrpc_client_SOURCES=	_mmrpc_client.c    mmrpc_common.c mmrpc_client.c 
mmrpc_client_LDADD=	$(top_builddir)/src/libgras.la

mmrpc_server_SOURCES=	_mmrpc_server.c    mmrpc_common.c                mmrpc_server.c
mmrpc_server_LDADD=	$(top_builddir)/src/libgras.la

# Take care of generatated sources
NAME=mmrpc
PROCESSES= client server
include $(top_srcdir)/examples/temps-gras-stub.mk

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
