/* $Id$ */

/* file trp (transport) - send/receive a bunch of bytes in SG realm         */

/* Note that this is only used to debug other parts of GRAS since message   */
/*  exchange in SG realm is implemented directly without mimicing real life */
/*  This would be terribly unefficient.                                     */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 Martin Quinson.                                       */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include <msg.h>

#include "gras_private.h"
#include "transport_private.h"

GRAS_LOG_EXTERNAL_CATEGORY(transport);
GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(trp_sg,transport);

/***
 *** Prototypes 
 ***/
gras_error_t gras_trp_sg_socket_client(gras_trp_plugin_t *self,
				       const char *host,
				       unsigned short port,
				       /* OUT */ gras_socket_t *sock);
gras_error_t gras_trp_sg_socket_server(gras_trp_plugin_t *self,
				       unsigned short port,
				       /* OUT */ gras_socket_t *sock);
void         gras_trp_sg_socket_close(gras_socket_t *sd);

gras_error_t gras_trp_sg_chunk_send(gras_socket_t *sd,
				    char *data,
				    size_t size);

gras_error_t gras_trp_sg_chunk_recv(gras_socket_t *sd,
				    char *data,
				    size_t size);

/* FIXME
  gras_error_t gras_trp_sg_flush(gras_socket_t *sd);
*/

/***
 *** Specific plugin part
 ***/
typedef struct {
  int placeholder; /* nothing plugin specific so far */
} gras_trp_sg_plug_specific_t;

/***
 *** Specific socket part
 ***/
typedef struct {
  int from_PID;    /* process which sent this message */
  int to_PID;      /* process to which this message is destinated */

  m_host_t to_host;   /* Who's on other side */
  m_channel_t to_chan;/* Channel on which the other side is earing */
} gras_trp_sg_sock_specific_t;


/***
 *** Code
 ***/

gras_error_t
gras_trp_sg_setup(gras_trp_plugin_t *plug) {

  gras_trp_sg_plug_specific_t *sg=malloc(sizeof(gras_trp_sg_plug_specific_t));
  if (!sg)
    RAISE_MALLOC;

  plug->socket_client = gras_trp_sg_socket_client;
  plug->socket_server = gras_trp_sg_socket_server;
  plug->socket_close  = gras_trp_sg_socket_close;

  plug->chunk_send    = gras_trp_sg_chunk_send;
  plug->chunk_recv    = gras_trp_sg_chunk_recv;

  plug->data      = sg; 

  return no_error;
}

gras_error_t gras_trp_sg_socket_client(gras_trp_plugin_t *self,
				       const char *host,
				       unsigned short port,
				       /* OUT */ gras_socket_t *dst){

  m_host_t peer;
  gras_hostdata_t *hd;
  int i;

  /* make sure this socket will reach someone */
  if (!(peer=MSG_get_host_by_name(host))) {
      fprintf(stderr,"GRAS: can't connect to %s: no such host.\n",host);
      return mismatch_error;
  }
  if (!(hd=(gras_hostdata_t *)MSG_host_get_data(peer))) {
      fprintf(stderr,"GRAS: can't connect to %s: no process on this host.\n",host);
      return mismatch_error;
  }
  for (i=0; i<hd->portLen && port != hd->port[i]; i++);
  if (i == hd->portLen) {
    fprintf(stderr,"GRAS: can't connect to %s:%d, no process listen on this port.\n",host,port);
    return mismatch_error;
  } 

  if (hd->raw[i] && !raw) {
    fprintf(stderr,"GRAS: can't connect to %s:%d in regular mode, the process listen in raw mode on this port.\n",host,port);
    return mismatch_error;
  }
  if (!hd->raw[i] && raw) {
    fprintf(stderr,"GRAS: can't connect to %s:%d in raw mode, the process listen in regular mode on this port.\n",host,port);
    return mismatch_error;
  }
    

  /* Create the socket */
  if (!(*sock=(gras_sock_t*)malloc(sizeof(gras_sock_t)))) {
      fprintf(stderr,"GRAS: openClientSocket: out of memory\n");
      return malloc_error;
  }    

  (*sock)->server_sock  = 0;
  (*sock)->raw_sock     = raw;
  (*sock)->from_PID     = MSG_process_self_PID();
  (*sock)->to_PID       = hd->proc[ hd->port2chan[i] ];
  (*sock)->to_host      = peer;
  (*sock)->to_port      = port;  
  (*sock)->to_chan      = hd->port2chan[i];

  /*
  fprintf(stderr,"GRAS: %s (PID %d) connects in %s mode to %s:%d (to_PID=%d).\n",
	  MSG_process_get_name(MSG_process_self()), MSG_process_self_PID(),
	  raw?"RAW":"regular",host,port,(*sock)->to_PID);
  */
}

gras_error_t gras_trp_sg_socket_server(gras_trp_plugin_t *self,
				       unsigned short port,
				       /* OUT */ gras_socket_t *dst){

  gras_hostdata_t *hd=(gras_hostdata_t *)MSG_host_get_data(MSG_host_self());
  gras_procdata_t *pd=(gras_procdata_t *)MSG_process_get_data(MSG_process_self());
  int port,i;
  const char *host=MSG_host_get_name(MSG_host_self());

  gras_assert0(hd,"Please run gras_process_init on each process");
  gras_assert0(pd,"Please run gras_process_init on each process");

  for (port=startingPort ; port <= endingPort ; port++) {
    for (i=0; i<hd->portLen && hd->port[i] != port; i++);
    if (i<hd->portLen && hd->port[i] == port)
      continue;

    /* port not used so far. Do it */
    if (i == hd->portLen) {
      /* need to enlarge the tables */
      if (hd->portLen++) {
	hd->port2chan=(int*)realloc(hd->port2chan,hd->portLen*sizeof(int));
	hd->port     =(int*)realloc(hd->port     ,hd->portLen*sizeof(int));
	hd->raw      =(int*)realloc(hd->raw      ,hd->portLen*sizeof(int));
      } else {
	hd->port2chan=(int*)malloc(hd->portLen*sizeof(int));
	hd->port     =(int*)malloc(hd->portLen*sizeof(int));
	hd->raw      =(int*)malloc(hd->portLen*sizeof(int));
      }
      if (!hd->port2chan || !hd->port || !hd->raw) {
	fprintf(stderr,"GRAS: PANIC: A malloc error did lose all ports attribution on this host\n");
	hd->portLen = 0;
	return malloc_error;
      }
    }
    hd->port2chan[ i ]=raw ? pd->rawChan : pd->chan;
    hd->port[ i ]=port;
    hd->raw[ i ]=raw;

    /* Create the socket */
    if (!(*sock=(gras_sock_t*)malloc(sizeof(gras_sock_t)))) {
      fprintf(stderr,"GRAS: openServerSocket: out of memory\n");
      return malloc_error;
    }    
    
    (*sock)->server_sock  = 1;
    (*sock)->raw_sock     = raw;
    (*sock)->from_PID     = -1;
    (*sock)->to_PID       = MSG_process_self_PID();
    (*sock)->to_host      = MSG_host_self();
    (*sock)->to_port      = port;  
    (*sock)->to_chan      = pd->chan;

    /*
    fprintf(stderr,"GRAS: '%s' (%d) ears on %s:%d%s (%p).\n",
	    MSG_process_get_name(MSG_process_self()), MSG_process_self_PID(),
	    host,port,raw? " (mode RAW)":"",*sock);
    */
    return no_error;
  }
  /* if we go out of the previous for loop, that's that we didn't find any
     suited port number */

  fprintf(stderr,
	  "GRAS: can't find an empty port between %d and %d to open a socket on host %s\n.",
	  startingPort,endingPort,host);
  return mismatch_error;
}

void gras_trp_sg_socket_close(gras_socket_t *sd){
  gras_hostdata_t *hd=(gras_hostdata_t *)MSG_host_get_data(MSG_host_self());
  int i;

  if (!sd) return;
  gras_assert0(hd,"Please run gras_process_init on each process");

  if (raw && !sd->raw_sock) {
      fprintf(stderr,"GRAS: gras_rawsock_close: Was passed a regular socket. Please use gras_sock_close()\n");
  }
  if (!raw && sd->raw_sock) {
      fprintf(stderr,"GRAS: grasSockClose: Was passed a raw socket. Please use gras_rawsock_close()\n");
  }
  if (sd->server_sock) {
    /* server mode socket. Un register it from 'OS' tables */
    for (i=0; 
	 i<hd->portLen && sd->to_port != hd->port[i]; 
	 i++);

    if (i==hd->portLen) {
      fprintf(stderr,"GRAS: closeSocket: The host does not know this server socket.\n");
    } else {
      memmove(&(hd->port[i]),      &(hd->port[i+1]),      (hd->portLen -i -1) * sizeof(int));
      memmove(&(hd->raw[i]),       &(hd->raw[i+1]),       (hd->portLen -i -1) * sizeof(int));
      memmove(&(hd->port2chan[i]), &(hd->port2chan[i+1]), (hd->portLen -i -1) * sizeof(int));
      hd->portLen--;
    }
  } 
  free(sd);
}

gras_error_t gras_trp_sg_chunk_send(gras_socket_t *sd,
				    char *data,
				    size_t size) {
  m_task_t task=NULL;
  static unsigned int count=0;
  char name[256];
  
  sprintf(name,"Chunk[%d]",count++);
  task=MSG_task_create(name,0,((double)size)/(1024.0*1024.0),NULL);

  if (MSG_task_put(task, sock->to_host,sock->to_chan) != MSG_OK) {
    RAISE(system_error,"Problem during the MSG_task_put");
  }

  return no_error;
}

gras_error_t gras_trp_sg_chunk_recv(gras_socket_t *sd,
				    char *data,
				    size_t size){
  gras_procdata_t *pd=
    (gras_procdata_t*)MSG_process_get_data(MSG_process_self());

  unsigned int bytesTotal=0;
  m_task_t task=NULL;

  if (MSG_task_get(&task, (m_channel_t) pd->rawChan) != MSG_OK) {
    fprintf(stderr,"GRAS: Error in MSG_task_get()\n");
    return unknown_error;
  }
  if (MSG_task_destroy(task) != MSG_OK) {
    fprintf(stderr,"GRAS: Error in MSG_task_destroy()\n");
    return unknown_error;
  }

  return no_error;
}

/*FIXME

gras_error_t gras_trp_sg_flush(gras_socket_t *sd){
  RAISE_UNIMPLEMENTED;
}
*/
