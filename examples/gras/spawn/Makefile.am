# Copyright (c) 2004-2007. The SimGrid team. All right reserved.

# This file is part of the SimGrid project. This is free software:
# You can redistribute and/or modify it under the terms of the
# GNU LGPL (v2.1) licence.


INCLUDES= -I$(top_srcdir)/include

EXTRA_DIST=spawn.xml spawn.h
include $(top_srcdir)/examples/gras/tests.mk

# AUTOMAKE variable definition
noinst_PROGRAMS=spawn_child spawn_father spawn_simulator

spawn_simulator_SOURCES=	_spawn_simulator.c spawn_father.c spawn_child.c spawn_common.c
spawn_simulator_LDADD=	$(top_builddir)/src/libsimgrid.la

spawn_child_SOURCES=	_spawn_child.c                  spawn_child.c spawn_common.c
spawn_child_LDADD=	$(top_builddir)/src/libgras.la

spawn_father_SOURCES=	_spawn_father.c  spawn_father.c spawn_child.c spawn_common.c
spawn_father_LDADD=	$(top_builddir)/src/libgras.la

# Take care of generatated sources
# (manually since the child is not in the deployment file, so we need an extra argument to the stub generator)

_spawn_father.c _spawn_child.c _spawn_simulator.c: $(srcdir)/spawn.xml $(top_builddir)/tools/gras/gras_stub_generator@EXEEXT@
	[ x@EXEEXT@ = x ] || exenv=wine; $$exenv $(top_builddir)/tools/gras/gras_stub_generator@EXEEXT@ --extra-process=child spawn $(srcdir)/spawn.xml

$(top_builddir)/tools/gras/gras_stub_generator@EXEEXT@:
	make -C $(top_builddir)/tools/gras/ gras_stub_generator@EXEEXT@

CLEANFILES = _spawn_simulator.c spawn.mk _spawn_father.c _spawn_child.c
MOSTLYCLEANFILES = spawn.trace

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
