AM_CFLAGS = -g

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src/include

bin_PROGRAMS = gras_stub_generator

LDADD_UTILS=$(top_builddir)/src/libsimgrid.la 
LDADD_RL=   $(top_builddir)/src/libsimgrid.la   

gras_stub_generator_SOURCES = stub_generator.c
gras_stub_generator_LDADD = $(LDADD_UTILS)
