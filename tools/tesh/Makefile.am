AM_CFLAGS = -g

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src/include

bin_PROGRAMS = tesh

tesh_SOURCES = tesh.c
tesh_LDADD   = $(top_builddir)/src/libsimgrid.la 

TESTS_ENVIRONMENT=./tesh
TESTS=basic.tesh  cd.tesh \
      broken-pipe.tesh segfault.tesh  timeout.tesh
      
#      non-blocking-IO.tesh -> blocks on AMD64

$(top_builddir)/src/libsimgrid.la: 
	make -C $(top_builddir)/src libsimgrid.la

include $(top_srcdir)/acmacro/dist-files.mk
