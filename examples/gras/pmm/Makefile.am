INCLUDES= -I$(top_srcdir)/include
TESTS= test_rl test_sg
EXTRA_DIST=pmm_deployment.xml  $(TESTS)

# AUTOMAKE variable definition
noinst_PROGRAMS=pmm_slave pmm_master pmm_simulator

pmm_simulator_SOURCES=_pmm_simulator.c pmm.c
pmm_simulator_LDADD=	$(top_builddir)/src/libsimgrid.la

pmm_slave_SOURCES=	_pmm_slave.c    pmm.c
pmm_slave_LDADD=	$(top_builddir)/src/libgras.la

pmm_master_SOURCES=	_pmm_master.c    pmm.c
pmm_master_LDADD=	$(top_builddir)/src/libgras.la

# Take care of generatated sources
NAME=pmm
PROCESSES= slave master
include $(top_srcdir)/examples/temps-gras-stub.mk

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
