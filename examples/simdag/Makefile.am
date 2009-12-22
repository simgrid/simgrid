# Copyright (c) 2004-2007. The SimGrid team. All right reserved.

# This file is part of the SimGrid project. This is free software:
# You can redistribute and/or modify it under the terms of the
# GNU LGPL (v2.1) licence.


INCLUDES = -I$(top_srcdir)/include
AM_CFLAGS = -g
EXTRA_DIST = 2clusters.xml 

CLEANFILES = sd_test.trace

TESTS_ENVIRONMENT = $(top_builddir)/tools/tesh/tesh

TESTS = test_simdag.tesh test_simdag2.tesh properties/test_prop.tesh metaxml/metaxml_test.tesh
noinst_PROGRAMS = sd_test sd_test2 properties/sd_prop metaxml/sd_meta\
 dax/dax_test

properties_sd_prop_SOURCES = properties/sd_prop.c
properties_sd_prop_LDADD = $(top_builddir)/src/libsimgrid.la

dax_dax_test_SOURCES = dax/dax_test.c
dax_dax_test_LDADD = $(top_builddir)/src/libsimgrid.la

metaxml_sd_meta_SOURCES = metaxml/sd_meta.c
metaxml_sd_meta_LDADD = $(top_builddir)/src/libsimgrid.la

sd_test_SOURCES = sd_test.c
sd_test_LDADD =	$(top_builddir)/src/libsimgrid.la

sd_test2_SOURCES = sd_test2.c
sd_test2_LDADD = $(top_builddir)/src/libsimgrid.la

EXTRA_DIST+=$(TESTS)
include $(top_srcdir)/acmacro/dist-files.mk
