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
#include "xbt/ex.h"
#include "transport_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(trp_buf,transport,
      "Generic buffered transport (works on top of TCP or Files, but not SG)");


static gras_trp_plugin_t _buf_super;

/***
 *** Prototypes 
 ***/
void hexa_print(const char*name, unsigned char *data, int size);   /* in gras.c */
   
void gras_trp_buf_socket_client(gras_trp_plugin_t self,
				gras_socket_t sock);
void gras_trp_buf_socket_server(gras_trp_plugin_t self,
				gras_socket_t sock);
gras_socket_t gras_trp_buf_socket_accept(gras_socket_t sock);

void         gras_trp_buf_socket_close(gras_socket_t sd);
  
void gras_trp_buf_chunk_send(gras_socket_t sd,
			     const char *data,
			     unsigned long int size);

void gras_trp_buf_chunk_recv(gras_socket_t sd,
			     char *data,
			     unsigned long int size,
			     unsigned long int bufsize);
void gras_trp_buf_flush(gras_socket_t sock);


/***
 *** Specific plugin part
 ***/

typedef struct {
  int junk;
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
   
   /* In SG, the 4 first bytes are for the chunk size as htonl'ed, so that we can send it in one shoot.
    * This is mandatory in SG because all emissions go to the same channel, so if we split them,
    * they can get mixed. */
  data->out.size = 0;
  data->out.data = xbt_malloc(data->buffsize);
  data->out.pos  = data->out.size;
   
  sock->bufdata = data;
}

/***
 *** Code
 ***/
void
gras_trp_buf_setup(gras_trp_plugin_t plug) {
  gras_trp_buf_plug_data_t *data =xbt_new(gras_trp_buf_plug_data_t,1);

  XBT_IN;
  _buf_super = gras_trp_plugin_get_by_name("tcp");

  DEBUG1("Derivate a buffer plugin from %s","tcp");

  plug->socket_client = gras_trp_buf_socket_client;
  plug->socket_server = gras_trp_buf_socket_server;
  plug->socket_accept = gras_trp_buf_socket_accept;
  plug->socket_close  = gras_trp_buf_socket_close;

  plug->chunk_send    = gras_trp_buf_chunk_send;
  plug->chunk_recv    = gras_trp_buf_chunk_recv;

  plug->flush         = gras_trp_buf_flush;

  plug->data = (void*)data;
  plug->exit = NULL;
}

void gras_trp_buf_socket_client(gras_trp_plugin_t self,
				/* OUT */ gras_socket_t sock){

  XBT_IN;
  _buf_super->socket_client(_buf_super,sock);
  sock->plugin = self;
  gras_trp_buf_init_sock(sock);
}

/**
 * gras_trp_buf_socket_server:
 *
 * Open a socket used to receive messages.
 */
void gras_trp_buf_socket_server(gras_trp_plugin_t self,
				/* OUT */ gras_socket_t sock){

  XBT_IN;
  _buf_super->socket_server(_buf_super,sock);
  sock->plugin = self;
  gras_trp_buf_init_sock(sock);
}

gras_socket_t
gras_trp_buf_socket_accept(gras_socket_t  sock) {

  gras_socket_t res;
      
  XBT_IN;
  res = _buf_super->socket_accept(sock);
  res->plugin = sock->plugin;
  gras_trp_buf_init_sock(res);
  XBT_OUT;
  return res;
}

void gras_trp_buf_socket_close(gras_socket_t sock){
  gras_trp_bufdata_t *data=sock->bufdata;

  XBT_IN;
  if (data->in.size!=data->in.pos) {
     WARN3("Socket closed, but %d bytes were unread (size=%d,pos=%d)",
	   data->in.size - data->in.pos,data->in.size, data->in.pos);
  }
   
  if (data->out.size!=data->out.pos) {
    DEBUG2("Flush the socket before closing (in=%d,out=%d)",data->in.size, data->out.size);
    gras_trp_buf_flush(sock);
  }
   
  if (data->in.data)
    free(data->in.data);
  if (data->out.data)
    free(data->out.data);
  free(data);

  _buf_super->socket_close(sock);
}

/**
 * gras_trp_buf_chunk_send:
 *
 * Send data on a buffered socket
 */
void
gras_trp_buf_chunk_send(gras_socket_t sock,
			const char *chunk,
			unsigned long int size) {

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
      gras_trp_buf_flush(sock);
  }

  XBT_OUT;
}

/**
 * gras_trp_buf_chunk_recv:
 *
 * Receive data on a buffered socket.
 */
void
gras_trp_buf_chunk_recv(gras_socket_t sock,
			char *chunk,
			unsigned long int size,
			unsigned long int bufsize) {

  xbt_ex_t e;
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
      TRY {
	_buf_super->chunk_recv(sock,(char*)&nextsize, 4,4);
      } CATCH(e) {
	RETHROW3("Unable to get the chunk size on %p (peer = %s:%d): %s",
		 sock,gras_socket_peer_name(sock),gras_socket_peer_port(sock));
      }
      data->in.size = (int)ntohl(nextsize);
      VERB1("Recv the chunk (size=%d)",data->in.size);
       
      _buf_super->chunk_recv(sock, data->in.data, data->in.size, data->in.size);
       
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
}

/**
 * gras_trp_buf_flush:
 *
 * Make sure the data is sent
 */
void
gras_trp_buf_flush(gras_socket_t sock) {
  int size;
  gras_trp_bufdata_t *data=sock->bufdata;
  XBT_IN;    
  
  DEBUG0("Flush");
  if (XBT_LOG_ISENABLED(trp_buf,xbt_log_priority_debug))
     hexa_print("chunck to send ",(unsigned char *) data->out.data,data->out.size);
  if ((data->out.size - data->out.pos) == 0) { 
     DEBUG2("Nothing to flush (size=%d; pos=%d)",data->out.size,data->out.pos);
     return;
  }
   
  size = (int)data->out.size - data->out.pos;
  DEBUG3("Send the size (=%d) to %s:%d",data->out.size-data->out.pos,
	 gras_socket_peer_name(sock),gras_socket_peer_port(sock));
  size = (int)htonl(size);
  _buf_super->chunk_send(sock,(char*) &size, 4);
      

  DEBUG3("Send the chunk (size=%d) to %s:%d",data->out.size,
	 gras_socket_peer_name(sock),gras_socket_peer_port(sock));
  _buf_super->chunk_send(sock, data->out.data, data->out.size);
  VERB1("Chunk sent (size=%d)",data->out.size);
  if (XBT_LOG_ISENABLED(trp_buf,xbt_log_priority_debug))
     hexa_print("chunck sent    ",(unsigned char *) data->out.data,data->out.size);
  data->out.size = 0;
}
