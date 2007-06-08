AM_CFLAGS = -g

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src/include -I$(top_srcdir)/src

bin_PROGRAMS = tesh

tesh_SOURCES = run_context.c run_context.h tesh.c tesh.h buff.h buff.c signal.c
tesh_LDADD   = $(top_builddir)/src/libgras.la 

TESTS_ENVIRONMENT=./tesh
TESTS=basic.tesh  cd.tesh \
      IO-broken-pipe.tesh IO-orders.tesh IO-bigsize.tesh \
      set-return.tesh   set-signal.tesh   set-timeout.tesh   set-ignore-output.tesh\
      catch-return.tesh catch-signal.tesh catch-timeout.tesh \
      catch-wrong-output.tesh \
      bg-basic.tesh bg-set-signal.tesh background.tesh
EXTRA_DIST=$(TESTS)

$(top_builddir)/src/libsimgrid.la: 
	make -C $(top_builddir)/src libsimgrid.la

include $(top_srcdir)/acmacro/dist-files.mk
