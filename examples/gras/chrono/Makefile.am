INCLUDES= -I$(top_srcdir)/include
AM_CFLAGS=-g
TESTS= test_rl test_sg
EXTRA_DIST=chrono.xml

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
