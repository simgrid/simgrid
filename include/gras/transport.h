
/* transport - low level communication (send/receive bunches of bytes)      */
/* module's public interface exported to end user.                          */

/* Copyright (c) 2004, 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_TRANSPORT_H
#define GRAS_TRANSPORT_H

#include "xbt/socket.h"

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

/** \brief Simply create a client socket (to speak to a remote host) */
XBT_PUBLIC(xbt_socket_t) gras_socket_client(const char *host,
                                             unsigned short port);
XBT_PUBLIC(xbt_socket_t) gras_socket_client_from_string(const char *host);
/** \brief Simply create a server socket (to ear from remote hosts speaking to you) */
XBT_PUBLIC(xbt_socket_t) gras_socket_server(unsigned short port);
XBT_PUBLIC(void) gras_socket_close(xbt_socket_t sd);
XBT_PUBLIC(void) gras_socket_close_voidp(void *sock);

/** \brief Create a client socket, full interface to all relevant settings */
XBT_PUBLIC(xbt_socket_t) gras_socket_client_ext(const char *host,
                                                unsigned short port,
                                                unsigned long int bufSize,
                                                int measurement);
/** \brief Create a server socket, full interface to all relevant settings */
XBT_PUBLIC(xbt_socket_t) gras_socket_server_ext(unsigned short port,
                                                unsigned long int bufSize,
                                                int measurement);
XBT_PUBLIC(xbt_socket_t)
    gras_socket_server_range(unsigned short minport, unsigned short maxport,
                             unsigned long int buf_size, int measurement);

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
XBT_PUBLIC(xbt_socket_t) gras_socket_client_from_file(const char *path);
XBT_PUBLIC(xbt_socket_t) gras_socket_server_from_file(const char *path);

/* @} */

void gras_trp_sg_setup(xbt_trp_plugin_t plug);
void gras_trp_file_setup(xbt_trp_plugin_t plug);

#endif                          /* GRAS_TRANSPORT_H */
