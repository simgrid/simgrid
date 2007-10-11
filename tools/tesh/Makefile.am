# Copyright (c) 2004-2007. The SimGrid team. All right reserved.

# This file is part of the SimGrid project. This is free software:
# You can redistribute and/or modify it under the terms of the
# GNU LGPL (v2.1) licence.


AM_CFLAGS = -g

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src/include -I$(top_srcdir)/src

bin_PROGRAMS = tesh

tesh_SOURCES = run_context.c run_context.h tesh.c tesh.h signal.c
tesh_LDADD   = $(top_builddir)/src/libgras.la 

TESTS_ENVIRONMENT=./tesh
TESTS=basic.tesh  cd.tesh \
      IO-broken-pipe.tesh IO-orders.tesh IO-bigsize.tesh \
      set-return.tesh   set-signal.tesh   set-timeout.tesh   set-ignore-output.tesh\
      catch-return.tesh catch-signal.tesh catch-timeout.tesh \
      catch-wrong-output.tesh \
      bg-basic.tesh bg-set-signal.tesh background.tesh
EXTRA_DIST=$(TESTS)

$(top_builddir)/src/libsimgrid.la: 
	make -C $(top_builddir)/src libsimgrid.la

include $(top_srcdir)/acmacro/dist-files.mk
