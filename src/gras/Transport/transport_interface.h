/* $Id$ */

/* transport - low level communication (send/receive bunches of bytes)      */

/* module's public interface exported within GRAS, but not to end user.     */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 Martin Quinson.                                       */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_TRP_INTERFACE_H
#define GRAS_TRP_INTERFACE_H

/***
 *** Main user functions
 ***/
gras_error_t gras_trp_chunk_send(gras_socket_t *sd,
				 char *data,
				 long int size);
gras_error_t gras_trp_chunk_recv(gras_socket_t *sd,
				 char *data,
				 long int size);
gras_error_t gras_trp_flush(gras_socket_t *sd);

/* Find which socket needs to be read next */
gras_error_t 
gras_trp_select(double timeout,
		gras_socket_t **dst);


/***
 *** Plugin mecanism 
 ***/

/* A plugin type */
typedef struct gras_trp_plugin_ gras_trp_plugin_t;

/* A plugin type */
struct gras_trp_plugin_ {
  char          *name;
 
  /* dst pointers are created and initialized with default values 
     before call to socket_client/server*/
  gras_error_t (*socket_client)(gras_trp_plugin_t *self,
				const char *host,
				unsigned short port,
				/* OUT */ gras_socket_t *dst);
  gras_error_t (*socket_server)(gras_trp_plugin_t *self,
				unsigned short port,
				/* OUT */ gras_socket_t *dst);
   
  gras_error_t (*socket_accept)(gras_socket_t  *sock,
				/* OUT */gras_socket_t **dst);
   
   
   /* socket_close() is responsible of telling the OS that the socket is over,
    but should not free the socket itself (beside the specific part) */
  void         (*socket_close)(gras_socket_t *sd);
    
  gras_error_t (*chunk_send)(gras_socket_t *sd,
			     const char *data,
			     long int size);
  gras_error_t (*chunk_recv)(gras_socket_t *sd,
			     char *Data,
			     long int size);

  /* flush has to make sure that the pending communications are achieved */
  gras_error_t (*flush)(gras_socket_t *sd);

  void          *data; /* plugin-specific data */
 
   /* exit is responsible for freeing data and telling the OS this plugin goes */
   /* exit=NULL, data gets freed. (ie exit function needed only when data contains pointers) */
  void         (*exit)(gras_trp_plugin_t *);
};

gras_error_t
gras_trp_plugin_get_by_name(const char *name,
			    gras_trp_plugin_t **dst);

#endif /* GRAS_TRP_INTERFACE_H */
