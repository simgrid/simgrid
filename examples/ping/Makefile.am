DISTCLEANFILES=Makefile.in
INCLUDES= -I$(top_srcdir)/src/include -g
EXTRA_DIST=ping_test
TESTS=test_rl test_sg

# AUTOMAKE variable definition
noinst_PROGRAMS=ping_simulator ping_client ping_server 

ping_simulator_SOURCES=	_ping_simulator.c ping.c
ping_client_SOURCES=	_ping_client.c ping.c
ping_server_SOURCES=	_ping_server.c ping.c


ping_simulator_LDADD=	$(top_srcdir)/src/base/libgrassg.a @LIBS_SimGrid@ @LIBS_XML@
ping_client_LDADD=	$(top_srcdir)/src/base/libgrasrl.a
ping_server_LDADD=	$(top_srcdir)/src/base/libgrasrl.a

# cleanup temps
CLEANFILES= _ping_simulator.c _ping_client.c _ping_server.c 

# generate temps
_ping_client.c _ping_server.c _ping_simulator.c: ping_deployment.txt $(top_srcdir)/src/examples/gras_stub_generator
	$(top_srcdir)/src/examples/gras_stub_generator ping ping_deployment.txt >/dev/null

