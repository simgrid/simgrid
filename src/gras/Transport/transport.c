/* $Id$ */

/* transport - low level communication                                      */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 Martin Quinson.                                       */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include <time.h>       /* time() */

#include "Transport/transport_private.h"


GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(transport,GRAS);

static gras_dict_t  *_gras_trp_plugins;     /* All registered plugins */
static void gras_trp_plugin_free(void *p); /* free one of the plugins */


gras_dynar_t *_gras_trp_sockets; /* all existing sockets */
static void gras_trp_socket_free(void *s); /* free one socket */

static fd_set FDread;

gras_error_t 
gras_trp_init(void){
  gras_error_t errcode;
  gras_trp_plugin_t *plug;
  
  /* make room for all socket ownership descriptions */
  TRY(gras_dynar_new(&_gras_trp_sockets, sizeof(gras_socket_t*), NULL));

  /* We do not ear for any socket for now */
  FD_ZERO(&FDread);
  
  /* make room for all plugins */
  TRY(gras_dict_new(&_gras_trp_plugins));

  /* TCP */
  TRY(gras_trp_tcp_init(&plug));
  TRY(gras_dict_set(_gras_trp_plugins, 
		    plug->name, plug, gras_trp_plugin_free));

  /* FILE */
  TRY(gras_trp_file_init(&plug));
  TRY(gras_dict_set(_gras_trp_plugins, 
		    plug->name, plug, gras_trp_plugin_free));

  return no_error;
}

void
gras_trp_exit(void){
  gras_dict_free(&_gras_trp_plugins);
  gras_dynar_free(_gras_trp_sockets);
}


void gras_trp_plugin_free(void *p) {
  gras_trp_plugin_t *plug = p;

  if (plug) {
    if (plug->free_specific && plug->specific)
      plug->free_specific(plug->specific);

    free(plug->name);
    free(plug);
  }
}


/**
 * gras_trp_socket_new:
 *
 * Malloc a new socket, and initialize it with defaults
 */
gras_error_t gras_trp_socket_new(int incoming,
				 gras_socket_t **dst) {

  gras_socket_t *sock;

  if (! (sock=malloc(sizeof(gras_socket_t))) )
    RAISE_MALLOC;

  sock->plugin = NULL;
  sock->sd     = -1;

  sock->incoming  = incoming? 1:0;
  sock->outgoing  = incoming? 0:1;
  sock->accepting = incoming? 1:0;
  DEBUG3("in=%c out=%c accept=%c",
	 sock->incoming?'y':'n', 
	 sock->outgoing?'y':'n',
	 sock->accepting?'y':'n');

  sock->port      = -1;
  sock->peer_port = -1;
  sock->peer_name = NULL;

  *dst = sock;
  return no_error;
}


/**
 * gras_socket_server:
 *
 * Opens a server socket and make it ready to be listened to.
 * In real life, you'll get a TCP socket.
 */
gras_error_t
gras_socket_server(unsigned short port,
		   /* OUT */ gras_socket_t **dst) {
 
  gras_error_t errcode;
  gras_trp_plugin_t *trp;
  gras_socket_t *sock;

  *dst = NULL;

  TRY(gras_trp_plugin_get_by_name(gras_if_RL() ? "TCP" : "SG",
				  &trp));

  /* defaults settings */
  TRY(gras_trp_socket_new(1,&sock));
  sock->plugin= trp;
  sock->port=port;

  /* Call plugin socket creation function */
  errcode = trp->socket_server(trp, port, sock);
  if (errcode != no_error) {
    free(sock);
    return errcode;
  }

  *dst = sock;
  /* Register this socket */
  errcode = gras_dynar_push(_gras_trp_sockets,dst);
  if (errcode != no_error) {
    free(sock);
    *dst = NULL;
    return errcode;
  }

  return no_error;
}

/**
 * gras_socket_client:
 *
 * Opens a client socket to a remote host.
 * In real life, you'll get a TCP socket.
 */
gras_error_t
gras_socket_client(const char *host,
		   unsigned short port,
		   /* OUT */ gras_socket_t **dst) {
 
  gras_error_t errcode;
  gras_trp_plugin_t *trp;
  gras_socket_t *sock;

  *dst = NULL;

  TRY(gras_trp_plugin_get_by_name(gras_if_RL() ? "TCP" : "SG",
				  &trp));

  /* defaults settings */
  TRY(gras_trp_socket_new(0,&sock));
  sock->plugin= trp;
  sock->peer_port = port;
  sock->peer_name = strdup(host?host:"localhost");

  /* plugin-specific */
  errcode= (* trp->socket_client)(trp, 
				  host ? host : "localhost", port,
				  sock);
  if (errcode != no_error) {
    free(sock);
    return errcode;
  }

  /* register socket */
  *dst = sock;
  errcode = gras_dynar_push(_gras_trp_sockets,dst);
  if (errcode != no_error) {
    free(sock);
    *dst = NULL;
    return errcode;
  }

  return no_error;
}

void gras_socket_close(gras_socket_t **sock) {
  gras_socket_t *sock_iter;
  int cursor;

  /* FIXME: Issue an event when the socket is closed */
  if (sock && *sock) {
    gras_dynar_foreach(_gras_trp_sockets,cursor,sock_iter) {
      if (*sock == sock_iter) {
	gras_dynar_cursor_rm(_gras_trp_sockets,&cursor);
	if ( (*sock)->plugin->socket_close) 
	  (* (*sock)->plugin->socket_close)(*sock);

	/* free the memory */
	free(*sock);
	*sock=NULL;
	return;
      }
    }
    WARN0("Ignoring request to free an unknown socket");
  }
}

/**
 * gras_trp_chunk_send:
 *
 * Send a bunch of bytes from on socket
 */
gras_error_t
gras_trp_chunk_send(gras_socket_t *sd,
		    char *data,
		    size_t size) {
  gras_assert1(sd->outgoing,
	       "Socket not suited for data send (outgoing=%c)",
	       sd->outgoing?'y':'n');
  gras_assert1(sd->plugin->chunk_send,
	       "No function chunk_send on transport plugin %s",
	       sd->plugin->name);
  return (*sd->plugin->chunk_send)(sd,data,size);
}
/**
 * gras_trp_chunk_recv:
 *
 * Receive a bunch of bytes from a socket
 */
gras_error_t 
gras_trp_chunk_recv(gras_socket_t *sd,
		    char *data,
		    size_t size) {
  gras_assert0(sd->incoming,
	       "Socket not suited for data receive");
  gras_assert1(sd->plugin->chunk_recv,
	       "No function chunk_recv on transport plugin %s",
	       sd->plugin->name);
  return (sd->plugin->chunk_recv)(sd,data,size);
}


gras_error_t
gras_trp_plugin_get_by_name(const char *name,
			    gras_trp_plugin_t **dst){

  return gras_dict_get(_gras_trp_plugins,name,(void**)dst);
}

int   gras_socket_my_port  (gras_socket_t *sock) {
  return sock->port;
}
int   gras_socket_peer_port(gras_socket_t *sock) {
  return sock->peer_port;
}
char *gras_socket_peer_name(gras_socket_t *sock) {
  return sock->peer_name;
}
