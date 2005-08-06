/* $Id$ */

/* transport - low level communication (send/receive bunches of bytes)      */

/* module's public interface exported within GRAS, but not to end user.     */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_TRP_INTERFACE_H
#define GRAS_TRP_INTERFACE_H

/***
 *** Main user functions
 ***/
void gras_trp_chunk_send(gras_socket_t sd,
			 char *data,
			 long int size);
void gras_trp_chunk_recv(gras_socket_t sd,
			 char *data,
			 long int size);
void gras_trp_flush(gras_socket_t sd);

/* Find which socket needs to be read next */
gras_socket_t gras_trp_select(double timeout);


/***
 *** Plugin mechanism 
 ***/

/* A plugin type */
typedef struct gras_trp_plugin_ s_gras_trp_plugin_t, *gras_trp_plugin_t;

/* A plugin type */
struct gras_trp_plugin_ {
  char          *name;
 
  /* dst pointers are created and initialized with default values
     before call to socket_client/server. 
     Retrive the info you need from there. */
  void (*socket_client)(gras_trp_plugin_t self,
			gras_socket_t    dst);
  void (*socket_server)(gras_trp_plugin_t self,
			gras_socket_t    dst);
   
  gras_socket_t (*socket_accept)(gras_socket_t  from);
   
   
   /* socket_close() is responsible of telling the OS that the socket is over,
    but should not free the socket itself (beside the specific part) */
  void (*socket_close)(gras_socket_t sd);
    
  void (*chunk_send)(gras_socket_t sd,
		     const char *data,
		     unsigned long int size);
  void (*chunk_recv)(gras_socket_t sd,
		     char *data,
		     unsigned long int size);

  /* flush has to make sure that the pending communications are achieved */
  void (*flush)(gras_socket_t sd);

  void *data; /* plugin-specific data */
 
   /* exit is responsible for freeing data and telling the OS this plugin goes */
   /* exit=NULL, data gets freed. (ie exit function needed only when data contains pointers) */
  void (*exit)(gras_trp_plugin_t);
};

gras_trp_plugin_t
gras_trp_plugin_get_by_name(const char *name);

/* Data of this module specific to each process
 * (used by sg_process.c to cleanup the SG channel cruft)
 */
typedef struct {
   /* SG only elements. In RL, they are part of the OS ;) */
   int chan;    /* Formated messages channel */
   int measChan; /* Unformated echange channel for performance measurement*/
   xbt_dynar_t sockets; /* all sockets known to this process */
   
} s_gras_trp_procdata_t,*gras_trp_procdata_t;

#endif /* GRAS_TRP_INTERFACE_H */
