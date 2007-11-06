# Copyright (c) 2004-2007. The SimGrid team. All right reserved.

# This file is part of the SimGrid project. This is free software:
# You can redistribute and/or modify it under the terms of the
# GNU LGPL (v2.1) licence.


INCLUDES= -I$(top_srcdir)/include

EXTRA_DIST=properties.xml properties.h
include $(top_srcdir)/examples/gras/tests.mk

# AUTOMAKE variable definition
noinst_PROGRAMS=properties_slave properties_master properties_simulator

properties_simulator_SOURCES=	_properties_simulator.c properties.c
properties_simulator_LDADD=	$(top_builddir)/src/libsimgrid.la

properties_slave_SOURCES=	_properties_slave.c    properties.c
properties_slave_LDADD=	$(top_builddir)/src/libgras.la

properties_master_SOURCES=	_properties_master.c    properties.c
properties_master_LDADD=	$(top_builddir)/src/libgras.la

# Take care of generatated sources
NAME=properties
PROCESSES= slave master
include $(top_srcdir)/examples/temps-gras-stub.mk

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
