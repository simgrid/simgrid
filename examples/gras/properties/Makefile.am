# Copyright (c) 2004-2007. The SimGrid team. All right reserved.

# This file is part of the SimGrid project. This is free software:
# You can redistribute and/or modify it under the terms of the
# GNU LGPL (v2.1) licence.


INCLUDES= -I$(top_srcdir)/include

EXTRA_DIST=
include $(top_srcdir)/examples/gras/tests.mk

# override EXTRA_DIST and TESTS since we do not have 32bits and 64bits variants of the sg test here
EXTRA_DIST=properties.xml test_sg test_rl
TESTS=test_sg test_rl


# AUTOMAKE variable definition
noinst_PROGRAMS=properties_alice properties_bob properties_simulator

properties_simulator_SOURCES=	_properties_simulator.c properties.c
properties_simulator_LDADD=	$(top_builddir)/src/libsimgrid.la

properties_alice_SOURCES=	_properties_alice.c    properties.c
properties_alice_LDADD=		$(top_builddir)/src/libgras.la

properties_bob_SOURCES=		_properties_bob.c    properties.c
properties_bob_LDADD=		$(top_builddir)/src/libgras.la

# Take care of generatated sources
NAME=properties
PROCESSES= alice bob
include $(top_srcdir)/examples/temps-gras-stub.mk

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
