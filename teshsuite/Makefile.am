INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src -I$(top_srcdir)/src/include
AM_CFLAGS = -g

# Declare all the variables to empty, so that each test set can add
# their content to them
CLEANFILES = 
EXTRA_DIST= 
noinst_PROGRAMS=

TESTS_ENVIRONMENT = $(top_builddir)/tools/tesh/tesh
TESTS =
XFAIL_TESTS=

LDADD_SG=$(abs_top_builddir)/src/libsimgrid.la
LDADD_RL=$(abs_top_builddir)/src/libgras.la

#################
### XBT tests ###
#################

  ## Try parallel logs ##
  #######################

TESTS += xbt/parallel_log_crashtest.tesh
noinst_PROGRAMS += xbt/parallel_log_crashtest 
xbt_parallel_log_crashtest_LDADD = $(LDADD_RL)

##################
### GRAS tests ###
##################

  ## Try structured communication facilities ##
  #############################################

CLEANFILES += gras/datadesc/datadesc_usage.out
EXTRA_DIST += \
  gras/datadesc/datadesc.little32_4 \
  gras/datadesc/datadesc.little64\
  gras/datadesc/datadesc.big32_8 gras/datadesc/datadesc.big32_8_4 gras/datadesc/datadesc.big32_2 \
  gras/datadesc/mk_datadesc_structs.pl
TESTS += \
  gras/datadesc/datadesc_mem.tesh          \
  gras/datadesc/datadesc_rw.tesh           \
  gras/datadesc/datadesc_r_little32_4.tesh \
  gras/datadesc/datadesc_r_little64.tesh   \
  \
  gras/datadesc/datadesc_r_big32_8.tesh      \
  gras/datadesc/datadesc_r_big32_8_4.tesh  \
  gras/datadesc/datadesc_r_big32_2.tesh
# Data sets still to regenerate:
XFAIL_TESTS+= \
  gras/datadesc/datadesc_r_little32.tesh   \
  gras/datadesc/datadesc_r_big32_8.tesh    \
  gras/datadesc/datadesc_r_big32_8_4.tesh  \
  gras/datadesc/datadesc_r_big32_2.tesh

noinst_PROGRAMS += gras/datadesc/datadesc_usage   
gras_datadesc_datadesc_usage_SOURCES=   gras/datadesc/datadesc_usage.c gras/datadesc/datadesc_structs.c
gras_datadesc_datadesc_usage_LDADD=     $(LDADD_RL)

gras/datadesc/datadesc_structs.c: gras/datadesc/mk_datadesc_structs.pl
	cd gras/datadesc ; perl ../$(top_srcdir)/teshsuite/gras/datadesc/mk_datadesc_structs.pl > datadesc_structs.c

  ## Ensures the gras_msg_handle semantic ##
  ##########################################
EXTRA_DIST += gras/msg_handle/msg_handle.xml
noinst_PROGRAMS+=\
  gras/msg_handle/msg_handle_client \
  gras/msg_handle/msg_handle_server \
  gras/msg_handle/msg_handle_simulator

gras_msg_handle_msg_handle_simulator_SOURCES= gras/msg_handle/msg_handle.c gras/msg_handle/_msg_handle_simulator.c
gras_msg_handle_msg_handle_client_SOURCES=    gras/msg_handle/msg_handle.c gras/msg_handle/_msg_handle_client.c
gras_msg_handle_msg_handle_server_SOURCES=    gras/msg_handle/msg_handle.c gras/msg_handle/_msg_handle_server.c

gras_msg_handle_msg_handle_simulator_LDADD=   $(LDADD_SG)
gras_msg_handle_msg_handle_client_LDADD=      $(LDADD_RL)
gras_msg_handle_msg_handle_server_LDADD=      $(LDADD_RL)

gras/msg_handle/_msg_handle_simulator.c gras/msg_handle/_msg_handle_client.c gras/msg_handle/_msg_handle_server.c : gras/msg_handle/msg_handle.c gras/msg_handle/msg_handle.xml $(top_builddir)/tools/gras/gras_stub_generator@EXEEXT@
	cd gras/msg_handle; ../../$(top_builddir)/tools/gras/gras_stub_generator@EXEEXT@ msg_handle $(srcdir)/msg_handle.xml

if GRAS_ARCH_32_BITS
  TESTS += gras/msg_handle/test_rl gras/msg_handle/test_sg_32
else
  TESTS += gras/msg_handle/test_rl gras/msg_handle/test_sg_64
endif
		    

include $(top_srcdir)/acmacro/dist-files.mk
