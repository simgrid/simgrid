/* $Id$ */

/* gras/socket.h - handling sockets in GRAS                                 */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */


#ifndef GRAS_SOCK_H
#define GRAS_SOCK_H

#include <stddef.h>    /* offsetof() */
#include <sys/types.h>  /* size_t */
#include <stdarg.h>


/*! C++ users need love */
#ifndef BEGIN_DECL
# ifdef __cplusplus
#  define BEGIN_DECL extern "C" {
# else
#  define BEGIN_DECL 
# endif
#endif

/*! C++ users need love */
#ifndef END_DECL
# ifdef __cplusplus
#  define END_DECL }
# else
#  define END_DECL 
# endif
#endif
/* End of cruft for C++ */

BEGIN_DECL

/****************************************************************************/
/* Openning/Maintaining/Closing connexions                                  */
/****************************************************************************/
/** Number of channel opened at most on a given host within SimGrid */
#define MAX_CHANNEL 10

/*! Type of a communication socket */
typedef struct gras_sock_s gras_sock_t;

/** 
 * gras_sock_client_open: 
 * @host: name of the host we want to connect to
 * @Param2: port on which we want to connect on this host
 * @sock: Newly created socket
 * @Returns: an errcode
 *
 * Attempts to establish a connection to the server listening to host:port 
 */
gras_error_t
gras_sock_client_open(const char *host, short port, 
		      /* OUT */ gras_sock_t **sock);

/** 
 * gras_sock_server_open:
 * @Param1: starting port 
 * @Param2: ending port 
 * @sock: Newly create socket
 * @Returns: an errcode
 * 
 * Attempts to bind to any port between #startingPort# and #endingPort#,
 * inclusive.
 *
 * You can get the port on which you connected using grasSockGetPort().
 */
gras_error_t
gras_sock_server_open(unsigned short startingPort, 
		      unsigned short endingPort,
		      /* OUT */ gras_sock_t **sock);

/**
 * gras_sock_close: 
 * @sock: The socket to close.
 * @Returns: an errcode
 *
 * Tears down a socket.
 */
gras_error_t gras_sock_close(gras_sock_t *sock);


/****************************************************************************/
/* Converting DNS name <-> IP                                               */
/****************************************************************************/
/**
 * gras_sock_get_peer_name:
 * @sd:
 * @Returns: the DNS name of the host connected to #sd#, or descriptive text if
 * #sd# is not an inter-host connection: returns NULL in case of error
 *
 * The value returned should not be freed.
 */
char *
gras_sock_get_peer_name(gras_sock_t *sd);

/**
 * gras_sock_get_peer_port:
 * @sd:
 * @Returns: the port number on the other side of socket sd. -1 is returned
 * if pipes or unknown
 */
unsigned short
gras_sock_get_peer_port(gras_sock_t *sd);

/**
 * gras_sock_get_peer_addr:
 * @sd:
 * @Returns: the IP address of the other side of this socket.
 *
 * can return NULL (out of memory condition).
 * Do not free the result.
 */
char *
gras_sock_get_peer_addr(gras_sock_t *sd);

/**
 * gras_sock_get_my_port:
 * @sd:
 * @Returns: the port number on the this side of socket sd. -1 is returned
 * if pipes or unknown
 */
unsigned short
gras_sock_get_my_port(gras_sock_t *sd);

/* **************************************************************************
 * Raw sockets and BW experiments (should be placed in another file)
 * **************************************************************************/
typedef struct gras_rawsock_s gras_rawsock_t;

/**
 * gras_rawsock_client_open:
 *
 * Establishes a connection to @machine : @port on which the buffer sizes have
 * been set to @bufSize bytes.
 *
 * Those sockets are meant to send raw data without any conversion, for example
 * for bandwidth tests.
 */
gras_error_t
gras_rawsock_client_open(const char *host, short port, unsigned int bufSize,
			 /* OUT */ gras_rawsock_t **sock);

/**
 * gras_rawsock_server_open:
 *
 * Open a connexion waiting for external input, on which the buffer sizes have
 * been set to @bufSize bytes.
 *
 * Those sockets are meant to send raw data without any conversion, for example
 * for bandwidth tests.
 */
gras_error_t
gras_rawsock_server_open(unsigned short startingPort, unsigned short endingPort,
			 unsigned int bufSize, /* OUT */ gras_rawsock_t **sock);

/**
 * gras_rawsock_close:
 *
 * Close a raw socket.
 *
 * Those sockets are meant to send raw data without any conversion, for example
 * for bandwidth tests.
 */
gras_error_t
gras_rawsock_close(gras_rawsock_t *sock);

/**
 * gras_rawsocket_get_my_port:
 * @sd:
 * @Returns: the port number on the this side of socket sd. -1 is returned
 * if pipes or unknown
 */
unsigned short 
gras_rawsocket_get_my_port(gras_rawsock_t *sd);

/**
 * gras_rawsocket_get_peer_port:
 * @sd:
 * @Returns: the port number on the other side of socket sd. -1 is returned
 * if pipes or unknown
 */
unsigned short
gras_rawsock_get_peer_port(gras_rawsock_t *sd);

/**
 * gras_rawsock_send:
 * @sock: on which raw socket to send the data
 * @expSize: total size of data sent
 * @msgSize: size of each message sent one after the other
 *
 * Send a raw bunch of data, for example for a bandwith test.
 */

gras_error_t
gras_rawsock_send(gras_rawsock_t *sd, unsigned int expSize, unsigned int msgSize);

/**
 * gras_rawsock_recv:
 * @sock: on which raw socket to read the data
 * @expSize: total size of data received
 * @msgSize: size of each message received one after the other
 * @timeout: time to wait for that data
 *
 * Receive a raw bunch of data, for example for a bandwith test.
 */

gras_error_t
gras_rawsock_recv(gras_rawsock_t *sd, unsigned int expSize, unsigned int msgSize, 
		  unsigned int timeout);


END_DECL

#endif /* GRAS_SOCK_H */

