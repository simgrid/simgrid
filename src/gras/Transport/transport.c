/* $Id$ */

/* transport - low level communication                                      */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 Martin Quinson.                                       */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include <time.h>       /* time() */
//#include <errno.h>

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
  TRY(gras_dict_insert(_gras_trp_plugins,plug->name, plug, gras_trp_plugin_free));

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

void gras_socket_close(gras_socket_t *sock) {
  gras_socket_t *sock_iter;
  int cursor;

  /* FIXME: Issue an event when the socket is closed */
  if (sock) {
    gras_dynar_foreach(_gras_trp_sockets,cursor,sock_iter) {
      if (sock == sock_iter) {
	gras_dynar_cursor_rm(_gras_trp_sockets,&cursor);
	if (sock->plugin->socket_close) 
	  (*sock->plugin->socket_close)(sock);

	/* free the memory */
	free(sock);
	return;
      }
    }
    WARNING0("Ignoring request to free an unknown socket");
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
  return (*sd->plugin->chunk_send)(sd,data,size);
}
/**
 * gras_trp_chunk_recv:
 *
 * Receive a bunch of bytes from a socket
 */
gras_error_t gras_trp_chunk_recv(gras_socket_t *sd,
				 char *data,
				 size_t size) {
  return (*sd->plugin->chunk_recv)(sd,data,size);
}


gras_error_t
gras_trp_plugin_get_by_name(const char *name,
			    gras_trp_plugin_t **dst){

  return gras_dict_retrieve(_gras_trp_plugins,name,(void**)dst);
}

