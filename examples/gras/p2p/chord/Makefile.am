# Copyright (c) 2004-2007. The SimGrid team. All right reserved.

# This file is part of the SimGrid project. This is free software:
# You can redistribute and/or modify it under the terms of the
# GNU LGPL (v2.1) licence.


INCLUDES= -I$(top_srcdir)/include
#TESTS= test_rl test_sg
EXTRA_DIST=chord.xml $(TESTS)

# AUTOMAKE variable definition
noinst_PROGRAMS=chord_node chord_simulator

chord_simulator_SOURCES=	_chord_simulator.c chord.c
chord_simulator_LDADD=	$(top_builddir)/src/libsimgrid.la

chord_node_SOURCES=	_chord_node.c chord.c
chord_node_LDADD=	$(top_builddir)/src/libgras.la

# Take care of generatated sources
NAME=chord
PROCESSES=node
include $(top_srcdir)/examples/temps-gras-stub.mk

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
