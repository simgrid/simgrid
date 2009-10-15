# Copyright (c) 2004-2007. The SimGrid team. All right reserved.

# This file is part of the SimGrid project. This is free software:
# You can redistribute and/or modify it under the terms of the
# GNU LGPL (v2.1) licence.


INCLUDES= -I$(top_srcdir)/include -I$(top_srcdir)/src/include
EXTRA_DIST=
include $(top_srcdir)/examples/gras/tests.mk

# AUTOMAKE variable definition
noinst_PROGRAMS=replay_worker replay_master replay_simulator

replay_simulator_SOURCES=_replay_simulator.c replay.c xbt_workload.c
replay_simulator_LDADD=	$(top_builddir)/src/libsimgrid.la

replay_master_SOURCES=	_replay_master.c    replay.c xbt_workload.c
replay_master_LDADD=	$(top_builddir)/src/libgras.la

replay_worker_SOURCES=	_replay_worker.c    replay.c xbt_workload.c
replay_worker_LDADD=	$(top_builddir)/src/libgras.la

# Take care of generatated sources
NAME=replay
PROCESSES= master worker
include $(top_srcdir)/examples/temps-gras-stub.mk

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
