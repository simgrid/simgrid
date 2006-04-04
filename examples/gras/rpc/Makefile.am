INCLUDES= -I$(top_srcdir)/include
TESTS= test_rl test_sg
EXTRA_DIST=rpc_deployment.xml $(TESTS)

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
