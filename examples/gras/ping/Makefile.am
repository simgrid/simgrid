INCLUDES= -I$(top_srcdir)/include
AM_CFLAGS=-g
TESTS= test_rl # test_sg
EXTRA_DIST=ping_deployment.txt $(TESTS)

# AUTOMAKE variable definition
check_PROGRAMS=ping_client ping_server 
# ping_simulator

#ping_simulator_SOURCES=_ping_simulator.c ping.c
#ping_simulator_LDADD=	$(top_builddir)/src/libsimgrid.la

ping_client_SOURCES=	_ping_client.c ping.c
ping_server_SOURCES=	_ping_server.c ping.c

ping_client_LDADD=	$(top_builddir)/src/libgras.la
ping_server_LDADD=	$(top_builddir)/src/libgras.la

# cleanup temps
MAINTAINERCLEANFILES = _ping_simulator.c _ping_client.c _ping_server.c 

# generate temps
NAME=ping
PROCESSES= client server

$(foreach proc, $(PROCESSES), _$(NAME)_$(proc).c) _ping_simulator.c: $(srcdir)/ping_deployment.txt $(srcdir)/../gras_stub_generator
	$(srcdir)/../gras_stub_generator ping ping_deployment.txt >/dev/null

