INCLUDES= -I$(top_srcdir)/include
#TESTS= test_rl test_sg
EXTRA_DIST=can_deployment.xml $(TESTS)

# AUTOMAKE variable definition
noinst_PROGRAMS=can_node can_simulator

can_simulator_SOURCES=	_can_simulator.c can.c
can_simulator_LDADD=	$(top_builddir)/src/libsimgrid.la

can_node_SOURCES=	_can_node.c can.c
can_node_LDADD=	$(top_builddir)/src/libgras.la

# Take care of generatated sources
NAME=can
PROCESSES=node
include $(top_srcdir)/examples/temps-gras-stub.mk

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
