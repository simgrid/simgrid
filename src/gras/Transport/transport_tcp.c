/* $Id$ */

/* tcp trp (transport) - send/receive a bunch of bytes from a tcp socket    */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 Martin Quinson.                                       */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras_private.h"
#include "transport_private.h"

GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(trp_tcp,transport);

typedef struct {
  int dummy;
} gras_trp_tcp_specific_t;

gras_error_t
gras_trp_tcp_init(void) {

  gras_trp_tcp_specific_t *specific = malloc(sizeof(gras_trp_tcp_specific_t));
  if (!specific)
    RAISE_MALLOC;

  return no_error;
}

void
gras_trp_tcp_exit(gras_trp_plugin_t *plugin) {
  gras_trp_tcp_specific_t *specific = (gras_trp_tcp_specific_t*)plugin->specific;
  free(specific);
}

gras_error_t gras_trp_tcp_socket_client(const char *host,
					unsigned short port,
					int raw, 
					unsigned int bufSize, 
					/* OUT */ gras_trp_sock_t **dst){
  RAISE_UNIMPLEMENTED;
}

gras_error_t gras_trp_tcp_socket_server(unsigned short port,
					int raw, 
					unsigned int bufSize, 
					/* OUT */ gras_trp_sock_t **dst){
  RAISE_UNIMPLEMENTED;
}

void gras_trp_tcp_socket_close(gras_trp_sock_t **sd){
  ERROR1("%s not implemented",__FUNCTION__);
  abort();
}

gras_error_t gras_trp_tcp_select(double timeOut,
				 gras_trp_sock_t **sd){
  RAISE_UNIMPLEMENTED;
}
  
gras_error_t gras_trp_tcp_bloc_send(gras_trp_sock_t *sd,
				    void *data,
				    size_t size,
				    double timeOut){
  RAISE_UNIMPLEMENTED;
}

gras_error_t gras_trp_tcp_bloc_recv(gras_trp_sock_t *sd,
				    void *data,
				    size_t size,
				    double timeOut){
  RAISE_UNIMPLEMENTED;
}

gras_error_t gras_trp_tcp_flush(gras_trp_sock_t *sd){
  RAISE_UNIMPLEMENTED;
}

