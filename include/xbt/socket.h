/* transport - low level communication (send/receive bunches of bytes)      */
/* module's public interface exported to end user.                          */

/* Copyright (c) 2004, 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_SOCKET_H
#define XBT_SOCKET_H

#include "xbt/misc.h"

/** \addtogroup XBT_sock
 *  \brief Socket handling
 *
 * The model of communications in XBT is very close to the BSD socket one.
 * To get two hosts exchanging data, one of them needs to open a
 * <i>server</i> socket on which it can listen for incoming messages and the
 * other one must connect a <i>client</i> socket onto the server one.
 *
 * The main difference is that you cannot exchange arbitrary bytes on
 * sockets, but messages. See the \ref GRAS_msg section for details.
 *
 * If you need an example of how to use sockets, check \ref GRAS_ex_ping.
 */

/** \defgroup XBT_sock_create Socket creation functions
 *  \ingroup XBT_sock
 *
 */
/* @{*/
/** \brief Opaque type describing a socket */
typedef struct s_xbt_socket *xbt_socket_t;
typedef struct s_xbt_trp_plugin s_xbt_trp_plugin_t, *xbt_trp_plugin_t;

XBT_PUBLIC(void) xbt_socket_new(int incoming, xbt_socket_t* dst);
XBT_PUBLIC(void) xbt_socket_new_ext(int incoming,
                                    xbt_socket_t* dst,
                                    xbt_trp_plugin_t plugin,
                                    unsigned long int buf_size,
                                    int measurement);
XBT_PUBLIC(void*) xbt_socket_get_data(xbt_socket_t sock);
XBT_PUBLIC(void) xbt_socket_set_data(xbt_socket_t sock, void* data);

/** \brief Simply create a client socket (to speak to a remote host) */
XBT_PUBLIC(xbt_socket_t) xbt_socket_tcp_client(const char *host,
                                               unsigned short port);
XBT_PUBLIC(xbt_socket_t) xbt_socket_tcp_client_from_string(const char *host);
/** \brief Simply create a server socket (to ear from remote hosts speaking to you) */
XBT_PUBLIC(xbt_socket_t) xbt_socket_tcp_server(unsigned short port);

/** \brief Create a client socket, full interface to all relevant settings */
XBT_PUBLIC(xbt_socket_t) xbt_socket_tcp_client_ext(const char *host,
                                                   unsigned short port,
                                                   unsigned long int bufSize,
                                                   int measurement);
/** \brief Create a server socket, full interface to all relevant settings */
XBT_PUBLIC(xbt_socket_t) xbt_socket_tcp_server_ext(unsigned short portcp_t,
                                                   unsigned long int bufSize,
                                                   int measurement);
XBT_PUBLIC(xbt_socket_t)
    xbt_socket_tcp_server_range(unsigned short minport, unsigned short maxport,
                                unsigned long int buf_size, int measurement);

XBT_PUBLIC(void) xbt_socket_close(xbt_socket_t sd);
XBT_PUBLIC(void) xbt_socket_close_voidp(void *sock);

/* @}*/
/** \defgroup XBT_sock_info Retrieving data about sockets and peers
 *  \ingroup XBT_sock
 * 
 * Who are you talking to? 
 */
/* @{*/

/** Get the port number on which this socket is connected on my side */
XBT_PUBLIC(int) xbt_socket_my_port(xbt_socket_t sock);

/** @brief Get the port number on which this socket is connected on remote side 
 *
 * This is the port declared on remote side with the
 * gras_socket_master() function (if any, or a random number being unique on
 * the remote host). If remote used gras_socket_master() more than once, the 
 * lastly declared number will be used here.
 *
 * Note to BSD sockets experts: With BSD sockets, the sockaddr 
 * structure allows you to retrieve the port of the client socket on
 * remote side, but it is of no use (from user perspective, it is
 * some random number above 6000). That is why XBT sockets differ
 * from BSD ones here. 
 */
XBT_PUBLIC(int) xbt_socket_peer_port(xbt_socket_t sock);
/** Get the host name of the remote side */
XBT_PUBLIC(const char *) xbt_socket_peer_name(xbt_socket_t sock);
/** Get the process name of the remote side */
XBT_PUBLIC(const char *) xbt_socket_peer_proc(xbt_socket_t sock);
/* @}*/

/** \defgroup XBT_sock_meas Using measurement sockets
 *  \ingroup XBT_sock
 * 
 * You may want to use sockets not to exchange valuable data (in messages), 
 * but to conduct some bandwidth measurements and related experiments. If so, try those measurement sockets.
 * 
 * You can only use those functions on sockets opened with the "measurement" boolean set to true.
 * 
 */
/* @{*/

XBT_PUBLIC(int) xbt_socket_is_meas(xbt_socket_t sock);
XBT_PUBLIC(void) xbt_socket_meas_send(xbt_socket_t peer,
                                      unsigned int timeout,
                                      unsigned long int msgSize,
                                      unsigned long int msgAmount);
XBT_PUBLIC(void) xbt_socket_meas_recv(xbt_socket_t peer,
                                      unsigned int timeout,
                                      unsigned long int msgSize,
                                      unsigned long int msgAmount);
XBT_PUBLIC(xbt_socket_t) xbt_socket_meas_accept(xbt_socket_t peer);

/* @}*/

/***
 *** Main user functions
 ***/
/* stable if we know the storage will keep as is until the next trp_flush */
XBT_PUBLIC(void) xbt_trp_send(xbt_socket_t sd, char *data, long int size,
                             int stable);
XBT_PUBLIC(void) xbt_trp_recv(xbt_socket_t sd, char *data, long int size);
XBT_PUBLIC(void) xbt_trp_flush(xbt_socket_t sd);

/* Find which socket needs to be read next */
XBT_PUBLIC(xbt_socket_t) xbt_trp_select(double timeout);

/* Set the peer process name (used by messaging layer) */
XBT_PUBLIC(void) xbt_socket_peer_proc_set(xbt_socket_t sock,
                                          char *peer_proc);

/** \defgroup XBT_sock_plugin Plugin mechanism
 *  \ingroup XBT_sock
 *
 * XBT provides a TCP plugin that implements TCP sockets.
 * You can also write your own plugin if you need another socket
 * implementation.
 */
/* @{*/

/* A plugin type */

struct s_xbt_trp_plugin {
  char *name;

  /* dst pointers are created and initialized with default values
     before call to socket_client/server.
     Retrieve the info you need from there. */
  void (*socket_client) (xbt_trp_plugin_t self, const char *host, int port, xbt_socket_t dst);
  void (*socket_server) (xbt_trp_plugin_t self, int port, xbt_socket_t dst);

  xbt_socket_t (*socket_accept) (xbt_socket_t from);

  /* Getting info about who's speaking */
  int (*my_port) (xbt_socket_t sd);
  int (*peer_port) (xbt_socket_t sd);
  const char* (*peer_name) (xbt_socket_t sd);
  const char* (*peer_proc) (xbt_socket_t sd);
  void (*peer_proc_set) (xbt_socket_t sd, char* peer_proc);

  /* socket_close() is responsible of telling the OS that the socket is over,
     but should not free the socket itself (beside the specific part) */
  void (*socket_close) (xbt_socket_t sd);

  /* send/recv may be buffered */
  void (*send) (xbt_socket_t sd,
                const char *data,
                unsigned long int size,
                int stable /* storage will survive until flush */ );
  int (*recv) (xbt_socket_t sd, char *data, unsigned long int size);
  /* raw_send/raw_recv is never buffered (use it for measurement stuff) */
  void (*raw_send) (xbt_socket_t sd,
                    const char *data, unsigned long int size);
  int (*raw_recv) (xbt_socket_t sd, char *data, unsigned long int size);

  /* flush has to make sure that the pending communications are achieved */
  void (*flush) (xbt_socket_t sd);

  void *data;                   /* plugin-specific data */

  /* exit is responsible for freeing data and telling to the OS that
     this plugin is gone.
     If exit is NULL, data gets brutally freed by the generic interface.
     (i.e. an exit function is only needed when data contains pointers) */
  void (*exit) (xbt_trp_plugin_t);
};

typedef void (*xbt_trp_setup_t) (xbt_trp_plugin_t dst);

XBT_PUBLIC(void) xbt_trp_plugin_new(const char *name, xbt_trp_setup_t setup);
XBT_PUBLIC(xbt_trp_plugin_t)
    xbt_trp_plugin_get_by_name(const char *name);

/* @}*/

#endif                          /* XBT_SOCKET_H */
