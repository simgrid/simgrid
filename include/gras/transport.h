/* $Id$ */

/* transport - low level communication (send/receive bunches of bytes)      */
/* module's public interface exported to end user.                          */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_TRANSPORT_H
#define GRAS_TRANSPORT_H

/** \addtogroup GRAS_sock
 *  \brief Socket handling (Communication facility).
 */

/** \name Socket creation functions
 *  \ingroup GRAS_sock
 */
/* @{*/
/** \brief Opaque type describing a socket */
typedef struct s_gras_socket *gras_socket_t;

/** \brief Simply create a client socket (to speak to a remote host) */
gras_socket_t gras_socket_client(const char *host, unsigned short port);
/** \brief Simply create a server socket (to ear from remote hosts speaking to you) */
gras_socket_t gras_socket_server(unsigned short port);
/** \brief Close socket */
void          gras_socket_close(gras_socket_t sd);

/** \brief Create a client socket, full interface to all relevant settings */
gras_socket_t gras_socket_client_ext(const char *host,
				     unsigned short port,
				     unsigned long int bufSize,
				     int measurement);
/** \brief Create a server socket, full interface to all relevant settings */
gras_socket_t gras_socket_server_ext(unsigned short port,
				     unsigned long int bufSize,
				     int measurement);
/* @}*/
/** \name Retrieving data about sockets and peers 
 *  \ingroup GRAS_sock
 * 
 * Who are you talking to?
 */
/* @{*/

/** Get the port number on which this socket is connected on my side */
int   gras_socket_my_port  (gras_socket_t sock);
/** Get the port number on which this socket is connected on remote side */
int   gras_socket_peer_port(gras_socket_t sock);
/** Get the host name of the remote side */
char *gras_socket_peer_name(gras_socket_t sock);
/* @}*/

/** \name Using measurement sockets
 *  \ingroup GRAS_sock
 * 
 * You may want to use sockets not to exchange valuable data (in messages), 
 * but to conduct some bandwidth measurements and related experiments. If so, try those measurement sockets.
 * 
 * You can only use those functions on sockets openned with the "measurement" boolean set to true.
 * 
 */
/* @{*/



int gras_socket_is_meas(gras_socket_t sock);
void gras_socket_meas_send(gras_socket_t peer, 
			   unsigned int timeout,
			   unsigned long int expSize, 
			   unsigned long int msgSize);
void gras_socket_meas_recv(gras_socket_t peer, 
			   unsigned int timeout,
			   unsigned long int expSize, 
			   unsigned long int msgSize);
gras_socket_t gras_socket_meas_accept(gras_socket_t peer);
            
/* @}*/

/** \name Using files as sockets
 *  \ingroup GRAS_sock
 * 
 * For debugging purpose, it is possible to deal with files as if they were sockets.
 * It can even be useful to store stuff in a portable manner, but writing messages to a file
 * may be strange...
 * 
 * \bug Don't use '-' on windows. this file represents stdin or stdout, but I failed to deal with it on windows.
 */
/* @{*/
/* debuging functions */
gras_socket_t gras_socket_client_from_file(const char*path);
gras_socket_t gras_socket_server_from_file(const char*path);
					  
/* @} */
   
#endif /* GRAS_TRANSPORT_H */
