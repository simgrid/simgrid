SUBDIRS=. Tests
DISTCLEANFILES=Makefile.in
INCLUDES= -I$(top_srcdir)/src/include \
          @CFLAGS_XML@ @CFLAGS_SimGrid@
EXTRA_DIST=                 \
	gras_private.h      \
	Core/dict_private.h 
#	DataDesc/parse.yy.l      


lib_LIBRARIES= libgrasrl.a libgrassg.a

#                     Core/dict_multi.c
#  Common/gras.c      Common/gras_datadesc.c        Common/gras_msg.c            
#   Messaging/messaging.c                       
#   Messaging/datadesc_simple.c                 
#   Transport/transport_sg.c 

COMMON_S=\
  \
  Core/module.c      Core/core_interface.h                                      \
  Core/log.c         Core/log_default_appender.c   Core/error.c                 \
  Core/dynar.c                                                                  \
  Core/dict.c        Core/dict_elm.c               Core/dict_cursor.c           \
  Core/set.c                                                                    \
  \
  Core/config.c                               \
  \
  Transport/transport.c    Transport/transport_private.h      \
  Transport/transport_plugin_file.c            \
  Transport/transport_plugin_tcp.c            \
  Transport/transport_plugin_sg.c             \
  \
  DataDesc/ddt_create.c          DataDesc/ddt_declare.c \
  DataDesc/ddt_remote.c          DataDesc/ddt_use.c     \
  DataDesc/cbps.c                DataDesc/datadesc.c    \
  DataDesc/datadesc_interface.h  DataDesc/datadesc_private.h 


#  DataDesc/datadesc.c                               \
#  DataDesc/tools.c                                  \
#  DataDesc/categories.h                             \
#  DataDesc/connection.h                             \
#  DataDesc/sequence.h                               \
#  DataDesc/net_driver.c      DataDesc/net_driver.h        \
#  DataDesc/net_driver_fd.c   DataDesc/net_interface_fd.h  \
#  DataDesc/type_driver_rl.c  DataDesc/type_interface_rl.h \
#  DataDesc/type_driver.c     DataDesc/type_driver.h       \
#  DataDesc/message.c         DataDesc/message.h           \
#  DataDesc/type.c            DataDesc/type.h              \
#  DataDesc/parse.c           DataDesc/parse.yy.h             DataDesc/parse.yy.c \
#  DataDesc/vars.c

DataDesc/parse.yy.c: DataDesc/parse.yy.l
	flex -o$@ -Pgras_ddt_ $^

libgrasrl_a_SOURCES= $(COMMON_S) \
  Transport/rl_transport.c       \
  Virtu/rl_process.c       Virtu/rl_time.c     Virtu/rl_conditional.c

libgrassg_a_SOURCES= $(COMMON_S) \
  Transport/sg_transport.c       \
  Virtu/sg_process.c       Virtu/sg_time.c     Virtu/sg_conditional.c

# RL/gras_rl.c RL/gras_rl.h FIXME
