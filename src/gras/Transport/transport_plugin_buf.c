/* $Id$ */

/* buf trp (transport) - buffered transport using the TCP one            */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
#include <string.h>       /* memset */

#include "portable.h"
#include "xbt/misc.h"
#include "xbt/sysdep.h"
#include "transport_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(trp_buf,transport,
      "Generic buffered transport (works on top of TCP or SG)");

/***
 *** Prototypes 
 ***/
xbt_error_t gras_trp_buf_socket_client(gras_trp_plugin_t *self,
					gras_socket_t sock);
xbt_error_t gras_trp_buf_socket_server(gras_trp_plugin_t *self,
					gras_socket_t sock);
xbt_error_t gras_trp_buf_socket_accept(gras_socket_t sock,
					gras_socket_t *dst);

void         gras_trp_buf_socket_close(gras_socket_t sd);
  
xbt_error_t gras_trp_buf_chunk_send(gras_socket_t sd,
				     const char *data,
				     long int size);

xbt_error_t gras_trp_buf_chunk_recv(gras_socket_t sd,
				     char *data,
				     long int size);
xbt_error_t gras_trp_buf_flush(gras_socket_t sock);


/***
 *** Specific plugin part
 ***/

typedef struct {
  gras_trp_plugin_t *super;
} gras_trp_buf_plug_data_t;

/***
 *** Specific socket part
 ***/

typedef struct {
  int size;
  char *data;
  int pos; /* for receive; not exchanged over the net */
} gras_trp_buf_t;

struct gras_trp_bufdata_{
  gras_trp_buf_t in;
  gras_trp_buf_t out;
  int buffsize;
};

void gras_trp_buf_init_sock(gras_socket_t sock) {
  gras_trp_bufdata_t *data=xbt_new(gras_trp_bufdata_t,1);
  
  XBT_IN;
  data->buffsize = 100 * 1024 ; /* 100k */ 

  data->in.size  = 0;
  data->in.data  = xbt_malloc(data->buffsize);
  data->in.pos   = 0; /* useless, indeed, since size==pos */
   
  data->out.size = 0;
  data->out.data = xbt_malloc(data->buffsize);
  data->out.pos  = 0;
   
  sock->bufdata = data;
}


/***
 *** Code
 ***/
xbt_error_t
gras_trp_buf_setup(gras_trp_plugin_t *plug) {
  xbt_error_t errcode;
  gras_trp_buf_plug_data_t *data =xbt_new(gras_trp_buf_plug_data_t,1);

  XBT_IN;
  TRY(gras_trp_plugin_get_by_name(gras_if_RL() ? "tcp" : "sg",
				  &(data->super)));
  DEBUG1("Derivate a buffer plugin from %s",gras_if_RL() ? "tcp" : "sg");

  plug->socket_client = gras_trp_buf_socket_client;
  plug->socket_server = gras_trp_buf_socket_server;
  plug->socket_accept = gras_trp_buf_socket_accept;
  plug->socket_close  = gras_trp_buf_socket_close;

  plug->chunk_send    = gras_trp_buf_chunk_send;
  plug->chunk_recv    = gras_trp_buf_chunk_recv;

  plug->flush         = gras_trp_buf_flush;

  plug->data = (void*)data;
  plug->exit = NULL;
  
  return no_error;
}

xbt_error_t gras_trp_buf_socket_client(gras_trp_plugin_t *self,
					/* OUT */ gras_socket_t sock){
  xbt_error_t errcode;
  gras_trp_plugin_t *super=((gras_trp_buf_plug_data_t*)self->data)->super;

  XBT_IN;
  TRY(super->socket_client(super,sock));
  sock->plugin = self;
  gras_trp_buf_init_sock(sock);
    
  return no_error;
}

/**
 * gras_trp_buf_socket_server:
 *
 * Open a socket used to receive messages.
 */
xbt_error_t gras_trp_buf_socket_server(gras_trp_plugin_t *self,
					/* OUT */ gras_socket_t sock){
  xbt_error_t errcode;
  gras_trp_plugin_t *super=((gras_trp_buf_plug_data_t*)self->data)->super;

  XBT_IN;
  TRY(super->socket_server(super,sock));
  sock->plugin = self;
  gras_trp_buf_init_sock(sock);
  return no_error;
}

xbt_error_t
gras_trp_buf_socket_accept(gras_socket_t  sock,
			   gras_socket_t *dst) {
  xbt_error_t errcode;
  gras_trp_plugin_t *super=((gras_trp_buf_plug_data_t*)sock->plugin->data)->super;
      
  XBT_IN;
  TRY(super->socket_accept(sock,dst));
  (*dst)->plugin = sock->plugin;
  gras_trp_buf_init_sock(*dst);
  XBT_OUT;
  return no_error;
}

void gras_trp_buf_socket_close(gras_socket_t sock){
  gras_trp_plugin_t *super=((gras_trp_buf_plug_data_t*)sock->plugin->data)->super;
  gras_trp_bufdata_t *data=sock->bufdata;

  XBT_IN;
  if (data->in.size || data->out.size)
    gras_trp_buf_flush(sock);
  if (data->in.data)
    free(data->in.data);
  if (data->out.data)
    free(data->out.data);
  free(data);

  super->socket_close(sock);
}

/**
 * gras_trp_buf_chunk_send:
 *
 * Send data on a TCP socket
 */
xbt_error_t 
gras_trp_buf_chunk_send(gras_socket_t sock,
			const char *chunk,
			long int size) {

  xbt_error_t errcode;
  gras_trp_bufdata_t *data=(gras_trp_bufdata_t*)sock->bufdata;
  int chunk_pos=0;

  XBT_IN;
  /* Let underneath plugin check for direction, we work even in duplex */
  xbt_assert0(size >= 0, "Cannot send a negative amount of data");

  while (chunk_pos < size) {
    /* size of the chunck to receive in that shot */
    long int thissize = min(size-chunk_pos,data->buffsize - data->out.size);
    DEBUG5("Set the chars %d..%ld into the buffer (size=%ld, ctn='%.*s')",
	   (int)data->out.size,
	   ((int)data->out.size) + thissize -1,
	   size, chunk_pos, chunk);

    memcpy(data->out.data + data->out.size, chunk + chunk_pos, thissize);

    data->out.size += thissize;
    chunk_pos      += thissize;
    DEBUG5("New pos = %d; Still to send = %ld of %ld; ctn sofar='%.*s'",
	   data->out.size,size-chunk_pos,size,(int)chunk_pos,chunk);

    if (data->out.size == data->buffsize) /* out of space. Flush it */
      TRY(gras_trp_buf_flush(sock));
  }

  XBT_OUT;
  return no_error;
}

/**
 * gras_trp_buf_chunk_recv:
 *
 * Receive data on a TCP socket.
 */
xbt_error_t 
gras_trp_buf_chunk_recv(gras_socket_t sock,
			char *chunk,
			long int size) {

  xbt_error_t errcode;
  gras_trp_plugin_t *super=((gras_trp_buf_plug_data_t*)sock->plugin->data)->super;
  gras_trp_bufdata_t *data=sock->bufdata;
  long int chunck_pos = 0;

  /* Let underneath plugin check for direction, we work even in duplex */
  xbt_assert0(sock, "Cannot recv on an NULL socket");
  xbt_assert0(size >= 0, "Cannot receive a negative amount of data");
  
  XBT_IN;

  while (chunck_pos < size) {
    /* size of the chunck to receive in that shot */
    long int thissize;

    if (data->in.size == data->in.pos) { /* out of data. Get more */
      int nextsize;
      DEBUG0("Recv the size");
      TRY(super->chunk_recv(sock,(char*)&nextsize, 4));
      data->in.size = (int)ntohl(nextsize);

      VERB1("Recv the chunk (size=%d)",data->in.size);
      TRY(super->chunk_recv(sock, data->in.data, data->in.size));
      data->in.pos=0;
    }
    
    thissize = min(size-chunck_pos ,  data->in.size - data->in.pos);
    DEBUG2("Get the chars %d..%ld out of the buffer",
	   data->in.pos,
	   data->in.pos + thissize - 1);
    memcpy(chunk+chunck_pos, data->in.data + data->in.pos, thissize);

    data->in.pos += thissize;
    chunck_pos   += thissize;
    DEBUG5("New pos = %d; Still to receive = %ld of %ld. Ctn so far='%.*s'",
	   data->in.pos,size - chunck_pos,size,(int)chunck_pos,chunk);
  }

  XBT_OUT;
  return no_error;
}

/**
 * gras_trp_buf_flush:
 *
 * Make sure the data is sent
 */
xbt_error_t 
gras_trp_buf_flush(gras_socket_t sock) {
  xbt_error_t errcode;
  int size;
  gras_trp_plugin_t *super=((gras_trp_buf_plug_data_t*)sock->plugin->data)->super;
  gras_trp_bufdata_t *data=sock->bufdata;

  XBT_IN;
  size = (int)htonl(data->out.size);
  DEBUG1("Send the size (=%d)",data->out.size);
  TRY(super->chunk_send(sock,(char*) &size, 4));

  DEBUG1("Send the chunk (size=%d)",data->out.size);
  TRY(super->chunk_send(sock, data->out.data, data->out.size));
  VERB1("Chunk sent (size=%d)",data->out.size);
  data->out.size = 0;
  return no_error;
}
