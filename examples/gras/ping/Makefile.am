INCLUDES= -I$(top_srcdir)/include
AM_CFLAGS=-g
TESTS= test_rl test_sg
EXTRA_DIST=ping_deployment.xml $(TESTS)

# AUTOMAKE variable definition
noinst_PROGRAMS=ping_client ping_server ping_simulator

ping_simulator_SOURCES=	_ping_simulator.c ping.c
ping_simulator_LDADD=	$(top_builddir)/src/libsimgrid.la

ping_client_SOURCES=	_ping_client.c ping.c
ping_client_LDADD=	$(top_builddir)/src/libgras.la

ping_server_SOURCES=	_ping_server.c ping.c
ping_server_LDADD=	$(top_builddir)/src/libgras.la

# cleanup temps
CLEANFILES = _ping_simulator.c _ping_client.c _ping_server.c \
             ping.Makefile.local ping.Makefile.remote ping.deploy.sh 

MOSTLYCLEANFILES = ping.trace

# generate temps
NAME=ping
PROCESSES= client server

$(foreach proc, $(PROCESSES), _$(NAME)_$(proc).c) _ping_simulator.c: $(srcdir)/ping_deployment.xml ../../../tools/gras/gras_stub_generator@EXEEXT@
	[ x@EXEEXT@ = x ] || exenv=wine; $$exenv ../../../tools/gras/gras_stub_generator@EXEEXT@ ping $(srcdir)/ping_deployment.xml >/dev/null

../../../tools/gras/gras_stub_generator@EXEEXT@:
	make -C   ../../../tools/gras/ gras_stub_generator@EXEEXT@

include $(top_srcdir)/acmacro/dist-files.mk
