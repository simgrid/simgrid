AM_CFLAGS = -g

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src/include -I$(top_srcdir)/src

bin_PROGRAMS = gras_stub_generator 
# skel_rewriter

gras_stub_generator_SOURCES = stub_generator.c unix_stub_generator.c windows_stub_generator.c
gras_stub_generator_LDADD   = $(top_builddir)/src/libsimgrid.la 

# skel_rewriter_SOURCES = skel_rewriter.c
# skel_rewriter_LDADD   = $(top_builddir)/src/libsimgrid.la 

$(top_builddir)/src/libsimgrid.la: 
	make -C $(top_builddir)/src libsimgrid.la

include $(top_srcdir)/acmacro/dist-files.mk
