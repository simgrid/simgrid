SUBDIRS=. Tests
DISTCLEANFILES=Makefile.in
INCLUDES= -I$(top_srcdir)/src/include -I../nws_portability/Include  \
          @CFLAGS_XML@ @CFLAGS_SimGrid@
EXTRA_DIST=                 \
	gras_private.h      \
	Core/dict_private.h \
	DataDesc/gs_private.h     \
	DataDesc/parse.yy.l      


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
  Messaging/transport.c    Messaging/transport_private.h      \
  Messaging/transport_sg.c Messaging/transport_tcp.c          \
  \
  Messaging/datadesc_simple.c                 \
  \
  DataDesc/datadesc.c                               \
  DataDesc/tools.c                                  \
  DataDesc/categories.h                             \
  DataDesc/connection.h                             \
  DataDesc/sequence.h                               \
  DataDesc/net_driver.c      DataDesc/net_driver.h        \
  DataDesc/net_driver_fd.c   DataDesc/net_interface_fd.h  \
  DataDesc/type_driver_rl.c  DataDesc/type_interface_rl.h \
  DataDesc/type_driver.c     DataDesc/type_driver.h       \
  DataDesc/message.c         DataDesc/message.h           \
  DataDesc/type.c            DataDesc/type.h              \
  DataDesc/parse.c           DataDesc/parse.yy.h             DataDesc/parse.yy.c \
  DataDesc/vars.c

DataDesc/parse.yy.c: DataDesc/parse.yy.l
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


