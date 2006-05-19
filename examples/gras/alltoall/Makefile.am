INCLUDES= -I$(top_srcdir)/include
TESTS= test_rl test_sg
EXTRA_DIST=alltoall_deployment.xml $(TESTS)

# AUTOMAKE variable definition
noinst_PROGRAMS=alltoall_node alltoall_simulator

alltoall_simulator_SOURCES=	_alltoall_simulator.c alltoall.c 
alltoall_simulator_LDADD=	$(top_builddir)/src/libsimgrid.la

alltoall_node_SOURCES=	_alltoall_node.c alltoall.c
alltoall_node_LDADD=	$(top_builddir)/src/libgras.la

# Take care of generatated sources
NAME=alltoall
PROCESSES= node
include $(top_srcdir)/examples/temps-gras-stub.mk

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
