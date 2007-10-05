# Copyright (c) 2004-2007. The SimGrid team. All right reserved.

# This file is part of the SimGrid project. This is free software:
# You can redistribute and/or modify it under the terms of the
# GNU LGPL (v2.1) licence.


INCLUDES= -I$(top_srcdir)/include
AM_CFLAGS=-g
EXTRA_DIST=chrono.xml
include $(top_srcdir)/examples/gras/tests.mk

# AUTOMAKE variable definition
noinst_PROGRAMS=chrono_simulator chrono_multiplier

chrono_simulator_SOURCES=	_chrono_simulator.c chrono.c
chrono_simulator_LDADD=	$(top_builddir)/src/libsimgrid.la

chrono_multiplier_SOURCES=	_chrono_multiplier.c chrono.c
chrono_multiplier_LDADD=	$(top_builddir)/src/libgras.la

# Take care of generatated sources
NAME=chrono
PROCESSES=multiplier
include $(top_srcdir)/examples/temps-gras-stub.mk

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
