INCLUDES= -I$(top_srcdir)/include
AM_CFLAGS=-g
TESTS=test_rl test_sg
EXTRA_DIST=ping_deployment.txt $(TESTS)

# check_SCRIPTS=$(TESTS)

# AUTOMAKE variable definition
if HAVE_SG
 check_PROGRAMS=ping_simulator ping_client ping_server 

 ping_simulator_SOURCES=_ping_simulator.c ping.c
 ping_simulator_LDADD=	$(top_builddir)/src/libgrassg.la
else
 check_PROGRAMS=ping_client ping_server
endif

ping_client_SOURCES=	_ping_client.c ping.c
ping_server_SOURCES=	_ping_server.c ping.c

ping_client_LDADD=	$(top_builddir)/src/libgrasrl.la
ping_server_LDADD=	$(top_builddir)/src/libgrasrl.la

# cleanup temps
MAINTAINERCLEANFILES = _ping_simulator.c _ping_client.c _ping_server.c 

# generate temps
_ping_client.c _ping_server.c _ping_simulator.c: ping_deployment.txt $(top_srcdir)/examples/gras_stub_generator
	$(top_srcdir)/examples/gras_stub_generator ping ping_deployment.txt >/dev/null

