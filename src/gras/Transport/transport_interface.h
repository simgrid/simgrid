/* $Id$ */

/* transport - low level communication (send/receive bunches of bytes)      */

/* module's public interface exported within GRAS, but not to end user.     */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 Martin Quinson.                                       */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_TRP_INTERFACE_H
#define GRAS_TRP_INTERFACE_H

#include "gras_private.h"

/***
 *** Main user functions
 ***/
gras_error_t gras_trp_chunk_send(gras_socket_t *sd,
				 char *data,
				 size_t size);
gras_error_t gras_trp_chunk_recv(gras_socket_t *sd,
				 char *data,
				 size_t size);

/* Find which socket needs to be read next */
gras_error_t 
gras_trp_select(double timeout,
		gras_socket_t **dst);


/***
 *** Module declaration 
 ***/
gras_error_t gras_trp_init(void);
void         gras_trp_exit(void);

/***
 *** Plugin mecanism 
 ***/

/* A plugin type */
typedef struct gras_trp_plugin_ gras_trp_plugin_t;


/**
 * e_gras_trp_plugin:
 * 
 * Caracterize each possible transport plugin
 */
typedef enum e_gras_trp_plugin {
   e_gras_trp_plugin_undefined = 0,
     
     e_gras_trp_plugin_tcp
} gras_trp_plugin_type_t;

/* A plugin type */
struct gras_trp_plugin_ {
  char          *name;
 
  gras_error_t (*socket_client)(gras_trp_plugin_t *self,
				const char *host,
				unsigned short port,
				unsigned int bufSize,
				/* OUT */ gras_socket_t **dst);
  gras_error_t (*socket_server)(gras_trp_plugin_t *self,
				unsigned short port,
				unsigned int bufSize,
				/* OUT */ gras_socket_t **dst);
  gras_error_t (*socket_accept)(gras_socket_t  *sock,
				/* OUT */gras_socket_t **dst);
   
   
   /* socket_close() is responsible of telling the OS that the socket is over,
    but should not free the socket itself (beside the specific part) */
  void         (*socket_close)(gras_socket_t *sd);
    
  gras_error_t (*chunk_send)(gras_socket_t *sd,
			     char *data,
			     size_t size);
  gras_error_t (*chunk_recv)(gras_socket_t *sd,
			     char *Data,
			     size_t size);

  void          *specific;
  void         (*free_specific)(void *);
};

gras_error_t
gras_trp_plugin_get_by_name(const char *name,
			    gras_trp_plugin_t **dst);

#endif /* GRAS_TRP_INTERFACE_H */
