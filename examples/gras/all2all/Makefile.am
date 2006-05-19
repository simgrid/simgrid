INCLUDES= -I$(top_srcdir)/include
TESTS= test_rl test_sg
EXTRA_DIST=all2all_deployment.xml $(TESTS)

# AUTOMAKE variable definition
noinst_PROGRAMS=all2all_node all2all_simulator

all2all_simulator_SOURCES=	_all2all_simulator.c all2all.c 
all2all_simulator_LDADD=	$(top_builddir)/src/libsimgrid.la

all2all_node_SOURCES=	_all2all_node.c all2all.c
all2all_node_LDADD=	$(top_builddir)/src/libgras.la

# Take care of generatated sources
NAME=all2all
PROCESSES= node
include $(top_srcdir)/examples/temps-gras-stub.mk

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
