/* $Id$ */

/* gras_sg.h - private interface for GRAS when using the simulator          */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_SG_H
#define GRAS_SG_H

#ifdef GRAS_RL_H
#error Impossible to load gras_sg.h and gras_rl.h at the same time
#endif

#include "gras_private.h"
#include <msg.h>

BEGIN_DECL

/* Declaration of the socket type */
struct gras_sock_s {
  int server_sock; /* boolean */
  int raw_sock;    /* boolean. If true, socket for raw send/recv. 
		               If false. socket for formated messages */

  int from_PID;    /* process which sent this message */
  int to_PID;      /* process to which this message is destinated */

  m_host_t to_host;   /* Who's on other side */
  int to_port;        /* port on which the server is earing */
  m_channel_t to_chan;/* Channel on which the other side is earing */
};
struct gras_rawsock_s {
  int server_sock; /* boolean */
  int raw_sock;    /* boolean. If true, socket for raw send/recv. 
		               If false. socket for formated messages */

  int from_PID;    /* process which sent this message */
  int to_PID;      /* process to which this message is destinated */

  m_host_t to_host;   /* Who's on other side */
  int to_port;        /* port on which the server is earing */
  m_channel_t to_chan;/* Channel on which the other side is earing */
};

/* Data for each host */
typedef struct {
  int proc[MAX_CHANNEL]; /* who's connected to each channel (its PID). If =0, then free */

  int portLen;
  int *port;             /* list of ports used by a server socket */
  int *port2chan;        /* the channel it points to */
  int *raw;              /* (boolean) the channel is in raw mode or for formatted messages */

} gras_hostdata_t;

/* prototype of private functions */
/**
 * _gras_sock_server_open:
 *
 * Open a server socket either in raw or regular mode, depending on the @raw argument.
 * In SG, bufSize is ignored.
 */
gras_error_t
_gras_sock_server_open(unsigned short startingPort, unsigned short endingPort,
		      int raw, unsigned int bufSize, /* OUT */ gras_sock_t **sock);

/**
 * _gras_sock_client_open:
 *
 * Open a client socket either in raw or regular mode, depending on the @raw argument.
 * In SG, bufSize is ignored.
 */
gras_error_t
_gras_sock_client_open(const char *host, short port, int raw, unsigned int bufSize,
		      /* OUT */ gras_sock_t **sock);


/**
 * _gras_sock_close:
 *
 * Close a socket which were opened either in raw or regular mode, depending on the
 * @raw argument.
 */
gras_error_t _gras_sock_close(int raw, gras_sock_t *sd);


END_DECL

#endif /* GRAS_SG_H */
