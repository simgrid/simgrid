/* $Id$ */

/* transport - low level communication (send/receive bunches of bytes)      */
/* module's public interface exported to end user.                          */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_TRANSPORT_H
#define GRAS_TRANSPORT_H

#include "xbt/error.h"

/** \addtogroup GRAS_sock
 *  \brief Socket handling (Communication facility).
 */

/** \name Socket creation functions
 *  \ingroup GRAS_sock
 */
/*@{*/
/** \brief Opaque type describing a socket */
typedef struct s_gras_socket *gras_socket_t;

/** \brief Simply create a client socket (to speak to a remote host) */
xbt_error_t gras_socket_client(const char *host,
				unsigned short port,
				/* OUT */ gras_socket_t *dst);
/** \brief Simply create a server socket (to ear from remote hosts speaking to you) */
xbt_error_t gras_socket_server(unsigned short port,
				/* OUT */ gras_socket_t *dst);
/** \brief Close socket */
void         gras_socket_close(gras_socket_t sd);

/** \brief Create a client socket, full interface to all relevant settings */
xbt_error_t gras_socket_client_ext(const char *host,
				    unsigned short port,
				    unsigned long int bufSize,
				    int raw, 
				    /* OUT */ gras_socket_t *dst);
/** \brief Create a server socket, full interface to all relevant settings */
xbt_error_t gras_socket_server_ext(unsigned short port,
				    unsigned long int bufSize,
				    int raw,
				    /* OUT */ gras_socket_t *dst);
/*@}*/
/** \name Retrieving data about sockets and peers 
 *  \ingroup GRAS_sock
 * 
 * Who are you talking to?
 */
/*@{*/

/** Get the port number on which this socket is connected on my side */
int   gras_socket_my_port  (gras_socket_t sock);
/** Get the port number on which this socket is connected on remote side */
int   gras_socket_peer_port(gras_socket_t sock);
/** Get the host name of the remote side */
char *gras_socket_peer_name(gras_socket_t sock);
/*@}*/

/** \name Using raw sockets
 *  \ingroup GRAS_sock
 * 
 * You may want to use sockets not to exchange valuable data (in messages), 
 * but to conduct some experiments such as bandwidth measurement. If so, try those raw sockets.
 * 
 * You can only use those functions on sockets openned with the "raw" boolean set to true.
 * 
 * \bug Raw sockets are not fully functionnal yet.
 */
/*@{*/

xbt_error_t gras_socket_raw_send(gras_socket_t peer, 
				  unsigned int timeout,
				  unsigned long int expSize, 
				  unsigned long int msgSize);
xbt_error_t gras_socket_raw_recv(gras_socket_t peer, 
				  unsigned int timeout,
				  unsigned long int expSize, 
				  unsigned long int msgSize);

/*@}*/

/** \name Using files as sockets
 *  \ingroup GRAS_sock
 * 
 * For debugging purpose, it is possible to deal with files as if they were sockets.
 * It can even be useful to store stuff in a portable manner, but writing messages to a file
 * may be strange...
 * 
 * \bug Don't use '-' on windows. this file represents stdin or stdout, but I failed to deal with it on windows.
 */
/*@{*/
/* debuging functions */
xbt_error_t gras_socket_client_from_file(const char*path,
					  /* OUT */ gras_socket_t *dst);
xbt_error_t gras_socket_server_from_file(const char*path,
					  /* OUT */ gras_socket_t *dst);
					  
/*@}*/
   
#endif /* GRAS_TRANSPORT_H */
