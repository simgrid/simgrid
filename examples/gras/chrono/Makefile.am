INCLUDES= -I$(top_srcdir)/include
AM_CFLAGS=-g
TESTS= test_rl test_sg
EXTRA_DIST=chrono_deployment.xml

# AUTOMAKE variable definition
noinst_PROGRAMS=chrono_simulator chrono_multiplier

chrono_simulator_SOURCES=	_chrono_simulator.c chrono.c
chrono_simulator_LDADD=	$(top_builddir)/src/libsimgrid.la

chrono_multiplier_SOURCES=	_chrono_multiplier.c chrono.c
chrono_multiplier_LDADD=	$(top_builddir)/src/libgras.la


# cleanup temps
CLEANFILES= _chrono_simulator.c _chrono_multiplier.c

# generate temps

# A rule to generate the source file each time the deployment file changes
_chrono_multiplier.c _chrono_simulator.c: chrono_deployment.xml ../../../tools/gras/gras_stub_generator@EXEEXT@
	[ x@EXEEXT@ = x ] || exenv=wine; $$exenv ../../../tools/gras/gras_stub_generator chrono $(srcdir)/chrono_deployment.xml >/dev/null

../../../tools/gras/gras_stub_generator@EXEEXT@:
	make -C   ../../../tools/gras/ gras_stub_generator@EXEEXT@

include $(top_srcdir)/acmacro/dist-files.mk
