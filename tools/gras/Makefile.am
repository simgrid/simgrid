AM_CFLAGS = -g
CFLAGS =

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src/include

check_PROGRAMS = stub_generator

LDADD_UTILS=$(top_builddir)/src/libgras.la 
LDADD_RL=   $(top_builddir)/src/libgras.la   

stub_generator_SOURCES = stub_generator.c
stub_generator_LDADD = $(LDADD_UTILS)