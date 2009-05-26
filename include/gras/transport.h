
/* $Id$ */

/* transport - low level communication (send/receive bunches of bytes)      */
/* module's public interface exported to end user.                          */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_TRANSPORT_H
#define GRAS_TRANSPORT_H

/** \addtogroup GRAS_sock
 *  \brief Socket handling
 *
 * The model of communications in GRAS is very close to the BSD socket one.
 * To get two hosts exchanging data, one of them need to open a
 * <i>server</i> socket on which it can listen for incoming messages and the
 * other one must connect a <i>client</i> socket onto the server one.
 *
 * The main difference is that you cannot exchange arbitrary bytes on
 * sockets, but messages. See the \ref GRAS_msg section for details.
 * 
 * If you need an example of how to use sockets, check \ref GRAS_ex_ping.
 *
 */

/** \defgroup GRAS_sock_create Socket creation functions
 *  \ingroup GRAS_sock
 *
 */
/* @{*/
/** \brief Opaque type describing a socket */
typedef struct s_gras_socket *gras_socket_t;

/** \brief Simply create a client socket (to speak to a remote host) */
XBT_PUBLIC(gras_socket_t) gras_socket_client(const char *host,
                                             unsigned short port);
XBT_PUBLIC(gras_socket_t) gras_socket_client_from_string(const char *host);
/** \brief Simply create a server socket (to ear from remote hosts speaking to you) */
XBT_PUBLIC(gras_socket_t) gras_socket_server(unsigned short port);
XBT_PUBLIC(void) gras_socket_close(gras_socket_t sd);

/** \brief Create a client socket, full interface to all relevant settings */
XBT_PUBLIC(gras_socket_t) gras_socket_client_ext(const char *host,
                                                 unsigned short port,
                                                 unsigned long int bufSize,
                                                 int measurement);
/** \brief Create a server socket, full interface to all relevant settings */
XBT_PUBLIC(gras_socket_t) gras_socket_server_ext(unsigned short port,
                                                 unsigned long int bufSize,
                                                 int measurement);
XBT_PUBLIC(gras_socket_t)
  gras_socket_server_range(unsigned short minport, unsigned short maxport,
                         unsigned long int buf_size, int measurement);

/* @}*/
/** \defgroup GRAS_sock_info Retrieving data about sockets and peers 
 *  \ingroup GRAS_sock
 * 
 * Who are you talking to? 
 */
/* @{*/

/** Get the port number on which this socket is connected on my side */
XBT_PUBLIC(int) gras_socket_my_port(gras_socket_t sock);
/** @brief Get the port number on which this socket is connected on remote side 
 *
 * This is the port declared on remote side with the
 * gras_socket_master() function (if any, or a random number being uniq on 
 * the remote host). If remote used gras_socket_master() more than once, the 
 * lastly declared number will be used here.
 *
 * Note to BSD sockets experts: With BSD sockets, the sockaddr 
 * structure allows you to retrieve the port of the client socket on
 * remote side, but it is of no use (from user perspective, it is
 * some random number above 6000). That is why GRAS sockets differ
 * from BSD ones here. 
 */

XBT_PUBLIC(int) gras_socket_peer_port(gras_socket_t sock);
/** Get the host name of the remote side */
XBT_PUBLIC(char *) gras_socket_peer_name(gras_socket_t sock);
/** Get the process name of the remote side */
XBT_PUBLIC(char *) gras_socket_peer_proc(gras_socket_t sock);
/* @}*/

/** \defgroup GRAS_sock_meas Using measurement sockets
 *  \ingroup GRAS_sock
 * 
 * You may want to use sockets not to exchange valuable data (in messages), 
 * but to conduct some bandwidth measurements and related experiments. If so, try those measurement sockets.
 * 
 * You can only use those functions on sockets openned with the "measurement" boolean set to true.
 * 
 */
/* @{*/



XBT_PUBLIC(int) gras_socket_is_meas(gras_socket_t sock);
XBT_PUBLIC(void) gras_socket_meas_send(gras_socket_t peer,
                                       unsigned int timeout,
                                       unsigned long int msgSize,
                                       unsigned long int msgAmount);
XBT_PUBLIC(void) gras_socket_meas_recv(gras_socket_t peer,
                                       unsigned int timeout,
                                       unsigned long int msgSize,
                                       unsigned long int msgAmount);
XBT_PUBLIC(gras_socket_t) gras_socket_meas_accept(gras_socket_t peer);

/* @}*/

/** \defgroup GRAS_sock_file Using files as sockets
 *  \ingroup GRAS_sock
 * 
 *
 * For debugging purpose, it is possible to deal with files as if they were sockets.
 * It can even be useful to store stuff in a portable manner, but writing messages to a file
 * may be strange...
 * 
 * \bug Don't use '-' on windows. this file represents stdin or stdout, but I failed to deal with it on windows.
 */
/* @{*/
/* debuging functions */
XBT_PUBLIC(gras_socket_t) gras_socket_client_from_file(const char *path);
XBT_PUBLIC(gras_socket_t) gras_socket_server_from_file(const char *path);

/* @} */

#endif /* GRAS_TRANSPORT_H */
