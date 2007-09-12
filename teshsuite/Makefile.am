INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src -I$(top_srcdir)/src/include
AM_CFLAGS = -g


CLEANFILES = gras/datadesc_usage.out datadesc_usage.out
EXTRA_DIST=run_tests.in \
  gras/datadesc.little32 gras/datadesc.little32_4 \
  gras/datadesc.little64\
  gras/datadesc.big32 gras/datadesc.big32_8_4 gras/datadesc.big32_2 \
  gras/mk_datadesc_structs.pl
  

TESTS_ENVIRONMENT = $(top_builddir)/tools/tesh/tesh
TESTS = xbt/parallel_log_crashtest.tesh     \
	gras/trp_tcp_usage.tesh             \
	gras/trp_file_usage.tesh            \
	gras/datadesc_mem.tesh          \
	gras/datadesc_rw.tesh           \
	gras/datadesc_r_little32.tesh   \
	gras/datadesc_r_little32_4.tesh \
	gras/datadesc_r_little64.tesh   
	
# Data sets still to regenerate:
#	gras/datadesc_usage_little32_4.tesh 
#           gras/datadesc_usage --read ./gras/datadesc.big32;     \
#           gras/datadesc_usage --read ./gras/datadesc.big32_8_4; \
#           gras/datadesc_usage --read ./gras/datadesc.big32_2;


noinst_PROGRAMS = \
       xbt/parallel_log_crashtest \
       \
       gras/trp_tcp_client   gras/trp_tcp_server   \
       gras/trp_file_client  gras/trp_file_server  \
       gras/datadesc_usage   
			       
LDADD_SG=$(abs_top_builddir)/src/libsimgrid.la
LDADD_RL=$(abs_top_builddir)/src/libgras.la

# Try parallel logs 
xbt_parallel_log_crashtest_LDADD = $(LDADD_RL)

# Try raw communication facilities
gras_trp_tcp_client_LDADD=     $(LDADD_RL)
gras_trp_tcp_server_LDADD=     $(LDADD_RL)
gras_trp_file_client_LDADD=    $(LDADD_RL)
gras_trp_file_server_LDADD=    $(LDADD_RL)

# Try structured communication facilities
gras_datadesc_usage_SOURCES=   gras/datadesc_usage.c gras/datadesc_structs.c
gras_datadesc_usage_LDADD=     $(LDADD_RL)

gras/datadesc_structs.c: gras/mk_datadesc_structs.pl
	cd gras ; perl ../$(top_srcdir)/teshsuite/gras/mk_datadesc_structs.pl > datadesc_structs.c


include $(top_srcdir)/acmacro/dist-files.mk
