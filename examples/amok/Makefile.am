# Copyright (c) 2004-2007. The SimGrid team. All right reserved.

# This file is part of the SimGrid project. This is free software:
# You can redistribute and/or modify it under the terms of the
# GNU LGPL (v2.1) licence.


INCLUDES= -I$(top_srcdir)/include
AM_CFLAGS=-g

EXTRA_DIST=
noinst_PROGRAMS=

TESTS_ENVIRONMENT = $(top_builddir)/tools/tesh/tesh
TESTS=
XFAIL_TESTS=

LDADD_SG=$(top_builddir)/src/libsimgrid.la
LDADD_RL=$(top_builddir)/src/libgras.la

###################
# Bandwidth tests #
###################
EXTRA_DIST+=bandwidth/bandwidth.xml
if GRAS_ARCH_32_BITS
  TESTS += bandwidth/bandwidth_rl.tesh bandwidth/bandwidth_sg_32.tesh
else
  TESTS += bandwidth/bandwidth_rl.tesh bandwidth/bandwidth_sg_64.tesh
endif
XFAIL_TESTS+=bandwidth/bandwidth_rl.tesh

noinst_PROGRAMS += \
  bandwidth/bandwidth_maestro   \
  bandwidth/bandwidth_sensor    \
  bandwidth/bandwidth_simulator
  
bandwidth_bandwidth_simulator_SOURCES=	bandwidth/bandwidth.c bandwidth/_bandwidth_simulator.c
bandwidth_bandwidth_maestro_SOURCES=	bandwidth/bandwidth.c bandwidth/_bandwidth_maestro.c
bandwidth_bandwidth_sensor_SOURCES=	bandwidth/bandwidth.c bandwidth/_bandwidth_sensor.c 

bandwidth_bandwidth_simulator_LDADD=	$(LDADD_SG)
bandwidth_bandwidth_maestro_LDADD=	$(LDADD_RL)
bandwidth_bandwidth_sensor_LDADD= 	$(LDADD_RL)


# Take care of generatated sources
bandwidth/_bandwidth_simulator.c bandwidth/_bandwidth_sensor.c bandwidth/_bandwidth_maestro.c : bandwidth/bandwidth.c bandwidth/bandwidth.xml $(top_builddir)/tools/gras/gras_stub_generator@EXEEXT@
	cd bandwidth; $(abs_top_builddir)/tools/gras/gras_stub_generator@EXEEXT@ bandwidth $(abs_top_srcdir)/examples/amok/bandwidth/bandwidth.xml

###################
# saturate tests #
###################
EXTRA_DIST+=saturate/saturate.xml

if GRAS_ARCH_32_BITS
  TESTS       += saturate/saturate_rl.tesh saturate/saturate_sg_32.tesh
else
  TESTS       += saturate/saturate_rl.tesh saturate/saturate_sg_64.tesh
  XFAIL_TESTS += saturate/saturate_sg_64.tesh
endif
XFAIL_TESTS += saturate/saturate_rl.tesh 


noinst_PROGRAMS += \
  saturate/saturate_maestro   \
  saturate/saturate_sensor    \
  saturate/saturate_simulator
  
saturate_saturate_simulator_SOURCES=	saturate/saturate.c saturate/_saturate_simulator.c
saturate_saturate_maestro_SOURCES=	saturate/saturate.c saturate/_saturate_maestro.c
saturate_saturate_sensor_SOURCES=	saturate/saturate.c saturate/_saturate_sensor.c 

saturate_saturate_simulator_LDADD=	$(LDADD_SG)
saturate_saturate_maestro_LDADD=	$(LDADD_RL)
saturate_saturate_sensor_LDADD= 	$(LDADD_RL)


# Take care of generatated sources
saturate/_saturate_simulator.c saturate/_saturate_sensor.c saturate/_saturate_maestro.c : saturate/saturate.c saturate/saturate.xml $(top_builddir)/tools/gras/gras_stub_generator@EXEEXT@
	cd saturate; $(abs_top_builddir)/tools/gras/gras_stub_generator@EXEEXT@ saturate $(abs_top_srcdir)/examples/amok/saturate/saturate.xml



#########
# Cruft #
#########
EXTRA_DIST+=$(TESTS)
include $(top_srcdir)/acmacro/dist-files.mk
