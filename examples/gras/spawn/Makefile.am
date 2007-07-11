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
NAME=spawn
PROCESSES= child father
include $(top_srcdir)/examples/temps-gras-stub.mk

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
