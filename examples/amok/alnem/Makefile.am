# Copyright (c) 2004-2007. The SimGrid team. All right reserved.

# This file is part of the SimGrid project. This is free software:
# You can redistribute and/or modify it under the terms of the
# GNU LGPL (v2.1) licence.


DISTCLEANFILES=Makefile.in
INCLUDES= -I$(top_srcdir)/src/include
#-I../../src -I../../nws_portability/Include @CFLAGS_SimGrid@
AM_CFLAGS=-g

check_PROGRAMS=alnem_simulator alnem_sensor alnem_maestro 

alnem_simulator_SOURCES=      _alnem_simulator.c alnem.c
alnem_sensor_SOURCES=         _alnem_sensor.c    alnem.c
alnem_maestro_SOURCES=        _alnem_maestro.c   alnem.c


alnem_simulator_LDADD=	  $(top_srcdir)/src/base/libgrassg.a $(top_srcdir)/src/modules/libgrasmodules.a @LIBS_SimGrid@ 
alnem_sensor_LDADD=       $(top_srcdir)/src/base/libgrasrl.a $(top_srcdir)/src/modules/libgrasmodules.a @LIBS_SimGrid@
alnem_maestro_LDADD=      $(top_srcdir)/src/base/libgrasrl.a $(top_srcdir)/src/modules/libgrasmodules.a @LIBS_SimGrid@

bin_PROGRAMS=alnem_builder
alnem_builder_LDADD=  @LIBS_SimGrid@ 

# Take care of generatated sources
NAME=alnem
PROCESSES=sensor maestro
include $(top_srcdir)/examples/temps-gras-stub.mk




