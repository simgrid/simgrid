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
#include "Virtu/virtu_sg.h"

GRAS_LOG_EXTERNAL_CATEGORY(transport);
GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(trp_sg,transport);

/***
 *** Prototypes 
 ***/
/* retrieve the port record associated to a numerical port on an host */
static gras_error_t find_port(gras_hostdata_t *hd, int port,
			      gras_sg_portrec_t *hpd);


gras_error_t gras_trp_sg_socket_client(gras_trp_plugin_t *self,
				       const char *host,
				       unsigned short port,
				       /* OUT */ gras_socket_t *sock);
gras_error_t gras_trp_sg_socket_server(gras_trp_plugin_t *self,
				       unsigned short port,
				       /* OUT */ gras_socket_t *sock);
void         gras_trp_sg_socket_close(gras_socket_t *sd);

gras_error_t gras_trp_sg_chunk_send(gras_socket_t *sd,
				    const char *data,
				    long int size);

gras_error_t gras_trp_sg_chunk_recv(gras_socket_t *sd,
				    char *data,
				    long int size);

/***
 *** Specific plugin part
 ***/
typedef struct {
  int placeholder; /* nothing plugin specific so far */
} gras_trp_sg_plug_data_t;


/***
 *** Code
 ***/
static gras_error_t find_port(gras_hostdata_t *hd, int port,
			      gras_sg_portrec_t *hpd) {
  int cpt;
  gras_sg_portrec_t pr;

  gras_assert0(hd,"Please run gras_process_init on each process");
  
  gras_dynar_foreach(hd->ports, cpt, pr) {
    if (pr.port == port) {
      memcpy(hpd,&pr,sizeof(gras_sg_portrec_t));
      return no_error;
    }
  }
  return mismatch_error;
}


gras_error_t
gras_trp_sg_setup(gras_trp_plugin_t *plug) {

  gras_trp_sg_plug_data_t *data=malloc(sizeof(gras_trp_sg_plug_data_t));

  if (!data)
    RAISE_MALLOC;

  plug->data      = data; 

  plug->socket_client = gras_trp_sg_socket_client;
  plug->socket_server = gras_trp_sg_socket_server;
  plug->socket_close  = gras_trp_sg_socket_close;

  plug->chunk_send    = gras_trp_sg_chunk_send;
  plug->chunk_recv    = gras_trp_sg_chunk_recv;

  plug->flush         = NULL; /* nothing cached */

  return no_error;
}

gras_error_t gras_trp_sg_socket_client(gras_trp_plugin_t *self,
				       const char *host,
				       unsigned short port,
				       /* OUT */ gras_socket_t *sock){

  gras_error_t errcode;

  m_host_t peer;
  gras_hostdata_t *hd;
  gras_trp_sg_sock_data_t *data;
  gras_sg_portrec_t pr;

  /* make sure this socket will reach someone */
  if (!(peer=MSG_get_host_by_name(host))) {
      fprintf(stderr,"GRAS: can't connect to %s: no such host.\n",host);
      return mismatch_error;
  }
  if (!(hd=(gras_hostdata_t *)MSG_host_get_data(peer))) {
    RAISE1(mismatch_error,
	   "can't connect to %s: no process on this host",
	   host);
  }
  errcode = find_port(hd,port,&pr);
  if (errcode != no_error && errcode != mismatch_error) 
    return errcode;

  if (errcode == mismatch_error) {
    RAISE2(mismatch_error,
	   "can't connect to %s:%d, no process listen on this port",
	   host,port);
  } 

  if (pr.raw && !sock->raw) {
    RAISE2(mismatch_error,
	   "can't connect to %s:%d in regular mode, the process listen "
	   "in raw mode on this port",host,port);
  }
  if (!pr.raw && sock->raw) {
    RAISE2(mismatch_error,
	   "can't connect to %s:%d in raw mode, the process listen "
	   "in regular mode on this port",host,port);
  }

  /* create the socket */
  if (!(data = malloc(sizeof(gras_trp_sg_sock_data_t))))
    RAISE_MALLOC;
  
  data->from_PID     = MSG_process_self_PID();
  data->to_PID       = hd->proc[ pr.tochan ];
  data->to_host      = peer;
  data->to_chan      = pr.tochan;
  
  sock->data = data;
  sock->incoming = 1;

  DEBUG6("%s (PID %d) connects in %s mode to %s:%d (to_PID=%d)",
	  MSG_process_get_name(MSG_process_self()), MSG_process_self_PID(),
	  sock->raw?"RAW":"regular",host,port,data->to_PID);

  return no_error;
}

gras_error_t gras_trp_sg_socket_server(gras_trp_plugin_t *self,
				       unsigned short port,
				       gras_socket_t *sock){

  gras_error_t errcode;

  gras_hostdata_t *hd=(gras_hostdata_t *)MSG_host_get_data(MSG_host_self());
  gras_procdata_t *pd=gras_procdata_get();
  gras_sg_portrec_t pr;
  gras_trp_sg_sock_data_t *data;
  
  const char *host=MSG_host_get_name(MSG_host_self());

  gras_assert0(hd,"Please run gras_process_init on each process");

  sock->accepting = 0; /* no such nuisance in SG */

  errcode = find_port(hd,port,&pr);
  switch (errcode) {
  case no_error: /* Port already used... */
    RAISE2(mismatch_error,
	   "can't listen on address %s:%d: port already in use\n.",
	   host,port);

  case mismatch_error: /* Port not used so far. Do it */
    pr.tochan = sock->raw ? pd->rawChan : pd->chan;
    pr.port   = port;
    pr.raw    = sock->raw;
    TRY(gras_dynar_push(hd->ports,&pr));
    
  default:
    return errcode;
  }
  
  /* Create the socket */
  if (!(data = malloc(sizeof(gras_trp_sg_sock_data_t))))
    RAISE_MALLOC;
  
  data->from_PID     = -1;
  data->to_PID       = MSG_process_self_PID();
  data->to_host      = MSG_host_self();
  data->to_chan      = pd->chan;
  
  sock->data = data;

  INFO6("'%s' (%d) ears on %s:%d%s (%p)",
    MSG_process_get_name(MSG_process_self()), MSG_process_self_PID(),
    host,port,sock->raw? " (mode RAW)":"",sock);

  return no_error;
}

void gras_trp_sg_socket_close(gras_socket_t *sock){
  gras_hostdata_t *hd=(gras_hostdata_t *)MSG_host_get_data(MSG_host_self());
  int cpt;
  
  gras_sg_portrec_t pr;

  if (!sock) return;
  gras_assert0(hd,"Please run gras_process_init on each process");

  if (sock->data)
    free(sock->data);

  if (sock->incoming) {
    /* server mode socket. Un register it from 'OS' tables */
    gras_dynar_foreach(hd->ports, cpt, pr) {
      DEBUG2("Check pr %d of %d", cpt, gras_dynar_length(hd->ports));
      if (pr.port == sock->port) {
	gras_dynar_cursor_rm(hd->ports, &cpt);
	return;
      }
    }
    WARN0("socket_close called on an unknown socket");
  }
}

typedef struct {
  int size;
  void *data;
} sg_task_data_t;

gras_error_t gras_trp_sg_chunk_send(gras_socket_t *sock,
				    const char *data,
				    long int size) {
  m_task_t task=NULL;
  static unsigned int count=0;
  char name[256];
  gras_trp_sg_sock_data_t *sock_data;
  sg_task_data_t *task_data;
  
  sock_data = (gras_trp_sg_sock_data_t *)sock->data;

  sprintf(name,"Chunk[%d]",count++);

  if (!(task_data=malloc(sizeof(sg_task_data_t)))) 
    RAISE_MALLOC;
  if (!(task_data->data=malloc(size)))
    RAISE_MALLOC;
  task_data->size = size;
  memcpy(task_data->data,data,size);

  task=MSG_task_create(name,0,((double)size)/(1024.0*1024.0),task_data);

  DEBUG5("send chunk %s from %s to  %s:%d (size=%ld)",
	 name, MSG_host_get_name(MSG_host_self()),
	 MSG_host_get_name(sock_data->to_host), sock_data->to_chan,size);
  if (MSG_task_put(task, sock_data->to_host,sock_data->to_chan) != MSG_OK) {
    RAISE0(system_error,"Problem during the MSG_task_put");
  }

  return no_error;
}

gras_error_t gras_trp_sg_chunk_recv(gras_socket_t *sock,
				    char *data,
				    long int size){
  gras_procdata_t *pd=gras_procdata_get();

  m_task_t task=NULL;
  sg_task_data_t *task_data;
  gras_trp_sg_sock_data_t *sock_data = sock->data;

  GRAS_IN;
  DEBUG4("recv chunk on %s ->  %s:%d (size=%ld)",
	 MSG_host_get_name(sock_data->to_host),
	 MSG_host_get_name(MSG_host_self()), sock_data->to_chan, size);
  if (MSG_task_get(&task, (sock->raw ? pd->rawChan : pd->chan)) != MSG_OK)
    RAISE0(unknown_error,"Error in MSG_task_get()");

  task_data = MSG_task_get_data(task);
  if (task_data->size != size)
    RAISE5(mismatch_error,
	   "Got %d bytes when %ld where expected (in %s->%s:%d)",
	   task_data->size, size,
	   MSG_host_get_name(sock_data->to_host),
	   MSG_host_get_name(MSG_host_self()), sock_data->to_chan);
  memcpy(data,task_data->data,size);
  free(task_data->data);
  free(task_data);

  if (MSG_task_destroy(task) != MSG_OK)
    RAISE0(unknown_error,"Error in MSG_task_destroy()");

  GRAS_OUT;
  return no_error;
}
