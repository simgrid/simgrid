/* $Id$ */

/* file trp (transport) - send/receive a bunch of bytes in SG realm         */

/* Note that this is only used to debug other parts of GRAS since message   */
/*  exchange in SG realm is implemented directly without mimicing real life */
/*  This would be terribly unefficient.                                     */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 Martin Quinson.                                       */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras_private.h"
#include "transport_private.h"

GRAS_LOG_EXTERNAL_CATEGORY(transport);
GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(trp_sg,transport);


/***
 *** Prototypes 
 ***/
void         gras_trp_sg_exit(gras_trp_plugin_t *plugin);
gras_error_t gras_trp_sg_socket_client(const char *host,
				       unsigned short port,
				       int raw, 
				       /* OUT */ gras_socket_t **dst);
gras_error_t gras_trp_sg_socket_server(unsigned short port,
				       int raw, 
				       /* OUT */ gras_socket_t **dst);
void         gras_trp_sg_socket_close(gras_socket_t **sd);
gras_error_t gras_trp_sg_select(double timeOut,
				gras_socket_t **sd);

gras_error_t gras_trp_sg_bloc_send(gras_socket_t *sd,
				   void *data,
				   size_t size,
				   double timeOut);

gras_error_t gras_trp_sg_bloc_recv(gras_socket_t *sd,
				   void *data,
				   size_t size,
				   double timeOut);
gras_error_t gras_trp_sg_flush(gras_socket_t *sd);

/***
 *** Specific plugin part
 ***/
typedef struct {
  int dummy;
} gras_trp_sg_specific_t;

/***
 *** Specific socket part
 ***/

/***
 *** Code
 ***/

gras_error_t
gras_trp_sg_init(gras_trp_plugin_t **dst) {

  gras_trp_sg_specific_t *specific = malloc(sizeof(gras_trp_sg_specific_t));
  if (!specific)
    RAISE_MALLOC;

  return no_error;
}

void
gras_trp_sg_exit(gras_trp_plugin_t *plugin) {
  gras_trp_sg_specific_t *specific = (gras_trp_sg_specific_t*)plugin->specific;
  free(specific);
}

gras_error_t gras_trp_sg_socket_client(const char *host,
				       unsigned short port,
				       int raw, 
				       /* OUT */ gras_socket_t **dst){
  RAISE_UNIMPLEMENTED;
}

gras_error_t gras_trp_sg_socket_server(unsigned short port,
				       int raw, 
				       /* OUT */ gras_socket_t **dst){
  RAISE_UNIMPLEMENTED;
}

void gras_trp_sg_socket_close(gras_socket_t **sd){
  ERROR1("%s not implemented",__FUNCTION__);
  abort();
}

gras_error_t gras_trp_sg_select(double timeOut,
				gras_socket_t **sd){
  RAISE_UNIMPLEMENTED;
}
  
gras_error_t gras_trp_sg_bloc_send(gras_socket_t *sd,
				   void *data,
				   size_t size,
				   double timeOut){
  RAISE_UNIMPLEMENTED;
}

gras_error_t gras_trp_sg_bloc_recv(gras_socket_t *sd,
				   void *data,
				   size_t size,
				   double timeOut){
  RAISE_UNIMPLEMENTED;
}

gras_error_t gras_trp_sg_flush(gras_socket_t *sd){
  RAISE_UNIMPLEMENTED;
}

