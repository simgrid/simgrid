SUBDIRS=. Tests
DISTCLEANFILES=Makefile.in
INCLUDES= -I$(top_srcdir)/src/include -I../nws_portability/Include  \
          @CFLAGS_XML@ @CFLAGS_SimGrid@
EXTRA_DIST=                 \
	gras_private.h      \
	Core/dict_private.h \
	gs/gs_private.h     \
	gs/parse.yy.l      


lib_LIBRARIES= libgrasrl.a libgrassg.a

noinst_LIBRARIES=libgrasutils.a

#                     Core/dict_multi.c
#  Common/gras.c      Common/gras_datadesc.c        Common/gras_msg.c            
#   Messaging/messaging.c                       

COMMON_S=\
  \
  Core/module.c                                                                 \
  Core/log.c         Core/log_default_appender.c   Core/error.c                 \
  Core/dynar.c                                                                  \
  Core/dict.c        Core/dict_elm.c               Core/dict_cursor.c           \
  \
  Core/config.c                               \
  \
  Messaging/transport.c    Messaging/transport.h \
  Messaging/transport_sg.c Messaging/transport_tcp.c   \
  \
  Messaging/datadesc_simple.c                 \
  \
  gs/datadesc.c                               \
  gs/tools.c                                  \
  gs/categories.h                             \
  gs/connection.h                             \
  gs/sequence.h                               \
  gs/net_driver.c      gs/net_driver.h        \
  gs/net_driver_fd.c   gs/net_interface_fd.h  \
  gs/type_driver_rl.c  gs/type_interface_rl.h \
  gs/type_driver.c     gs/type_driver.h       \
  gs/message.c         gs/message.h           \
  gs/type.c            gs/type.h              \
  gs/parse.c           gs/parse.yy.h             gs/parse.yy.c \
  gs/vars.c

gs/parse.yy.c: gs/parse.yy.l
	flex -o$@ -Pgs_parse_ $^

COMMON_L=\
 $(foreach file,\
  diagnostic formatutil strutil osutil, \
  $(top_srcdir)/src/nws_portability/build-@host@/obj/$(file).o)

libgrasutils_a_SOURCES = $(COMMON_S) Tests/gras_dummy.c
libgrasutils_a_LIBADD = $(COMMON_L)



# RL/gras_rl.c RL/gras_rl.h FIXME
libgrasrl_a_SOURCES= $(COMMON_S) Messaging/messaging_rl.c
libgrasrl_a_LIBADD= \
 $(foreach file,\
  exp_smooth forc median mse_forc protocol \
  dnsutil fbuff forecast_api last_value run_mean timeouts, \
  $(top_srcdir)/src/nws_portability/build-@host@/obj/$(file).o) \
 $(COMMON_L)

	
# SG/gras_sg.c SG/gras_sg.h FIXME	
libgrassg_a_SOURCES= $(COMMON_S) Messaging/messaging_sg.c
libgrassg_a_LIBADD= \
 $(COMMON_L)


