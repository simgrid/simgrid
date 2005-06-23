/* $Id$ */

/* file trp (transport) - send/receive a bunch of bytes in SG realm         */

/* Note that this is only used to debug other parts of GRAS since message   */
/*  exchange in SG realm is implemented directly without mimicing real life */
/*  This would be terribly unefficient.                                     */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg/msg.h"

#include "transport_private.h"
#include "gras/Virtu/virtu_sg.h"

XBT_LOG_EXTERNAL_CATEGORY(transport);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(trp_sg,transport,"SimGrid pseudo-transport");

/***
 *** Prototypes 
 ***/
void hexa_print(unsigned char *data, int size);   /* in gras.c */

/* retrieve the port record associated to a numerical port on an host */
static xbt_error_t find_port(gras_hostdata_t *hd, int port,
			      gras_sg_portrec_t *hpd);


xbt_error_t gras_trp_sg_socket_client(gras_trp_plugin_t *self,
				       /* OUT */ gras_socket_t sock);
xbt_error_t gras_trp_sg_socket_server(gras_trp_plugin_t *self,
				       /* OUT */ gras_socket_t sock);
void         gras_trp_sg_socket_close(gras_socket_t sd);

xbt_error_t gras_trp_sg_chunk_send(gras_socket_t sd,
				    const char *data,
				    long int size);

xbt_error_t gras_trp_sg_chunk_recv(gras_socket_t sd,
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
static xbt_error_t find_port(gras_hostdata_t *hd, int port,
			      gras_sg_portrec_t *hpd) {
  int cpt;
  gras_sg_portrec_t pr;

  xbt_assert0(hd,"Please run gras_process_init on each process");
  
  xbt_dynar_foreach(hd->ports, cpt, pr) {
    if (pr.port == port) {
      memcpy(hpd,&pr,sizeof(gras_sg_portrec_t));
      return no_error;
    }
  }
  return mismatch_error;
}


xbt_error_t
gras_trp_sg_setup(gras_trp_plugin_t *plug) {

  gras_trp_sg_plug_data_t *data=xbt_new(gras_trp_sg_plug_data_t,1);

  plug->data      = data; 

  plug->socket_client = gras_trp_sg_socket_client;
  plug->socket_server = gras_trp_sg_socket_server;
  plug->socket_close  = gras_trp_sg_socket_close;

  plug->chunk_send    = gras_trp_sg_chunk_send;
  plug->chunk_recv    = gras_trp_sg_chunk_recv;

  plug->flush         = NULL; /* nothing cached */

  return no_error;
}

xbt_error_t gras_trp_sg_socket_client(gras_trp_plugin_t *self,
				       /* OUT */ gras_socket_t sock){

  xbt_error_t errcode;

  m_host_t peer;
  gras_hostdata_t *hd;
  gras_trp_sg_sock_data_t *data;
  gras_sg_portrec_t pr;

  /* make sure this socket will reach someone */
  if (!(peer=MSG_get_host_by_name(sock->peer_name))) {
      fprintf(stderr,"GRAS: can't connect to %s: no such host.\n",sock->peer_name);
      return mismatch_error;
  }
  if (!(hd=(gras_hostdata_t *)MSG_host_get_data(peer))) {
    RAISE1(mismatch_error,
	   "can't connect to %s: no process on this host",
	   sock->peer_name);
  }
  errcode = find_port(hd,sock->peer_port,&pr);
  if (errcode != no_error && errcode != mismatch_error) 
    return errcode;

  if (errcode == mismatch_error) {
    RAISE2(mismatch_error,
	   "can't connect to %s:%d, no process listen on this port",
	   sock->peer_name,sock->peer_port);
  } 

  if (pr.meas && !sock->meas) {
    RAISE2(mismatch_error,
	   "can't connect to %s:%d in regular mode, the process listen "
	   "in meas mode on this port",sock->peer_name,sock->peer_port);
  }
  if (!pr.meas && sock->meas) {
    RAISE2(mismatch_error,
	   "can't connect to %s:%d in meas mode, the process listen "
	   "in regular mode on this port",sock->peer_name,sock->peer_port);
  }

  /* create the socket */
  data = xbt_new(gras_trp_sg_sock_data_t,1);
  data->from_PID     = MSG_process_self_PID();
  data->to_PID       = hd->proc[ pr.tochan ];
  data->to_host      = peer;
  data->to_chan      = pr.tochan;
  
  sock->data = data;
  sock->incoming = 1;

  DEBUG6("%s (PID %d) connects in %s mode to %s:%d (to_PID=%d)",
	  MSG_process_get_name(MSG_process_self()), MSG_process_self_PID(),
	  sock->meas?"meas":"regular",
	 sock->peer_name,sock->peer_port,data->to_PID);

  return no_error;
}

xbt_error_t gras_trp_sg_socket_server(gras_trp_plugin_t *self,
				       gras_socket_t sock){

  xbt_error_t errcode;

  gras_hostdata_t *hd=(gras_hostdata_t *)MSG_host_get_data(MSG_host_self());
  gras_trp_procdata_t pd=(gras_trp_procdata_t)gras_libdata_get("gras_trp");
  gras_sg_portrec_t pr;
  gras_trp_sg_sock_data_t *data;
  
  const char *host=MSG_host_get_name(MSG_host_self());

  xbt_assert0(hd,"Please run gras_process_init on each process");

  sock->accepting = 0; /* no such nuisance in SG */

  errcode = find_port(hd,sock->port,&pr);
  switch (errcode) {
  case no_error: /* Port already used... */
    RAISE2(mismatch_error,
	   "can't listen on address %s:%d: port already in use\n.",
	   host,sock->port);
    break;

  case mismatch_error: /* Port not used so far. Do it */
    pr.tochan = sock->meas ? pd->measChan : pd->chan;
    pr.port   = sock->port;
    pr.meas    = sock->meas;
    xbt_dynar_push(hd->ports,&pr);
    break;
    
  default:
    return errcode;
  }
  
  /* Create the socket */
  data = xbt_new(gras_trp_sg_sock_data_t,1);
  data->from_PID     = -1;
  data->to_PID       = MSG_process_self_PID();
  data->to_host      = MSG_host_self();
  data->to_chan      = pd->chan;
  
  sock->data = data;

  VERB6("'%s' (%d) ears on %s:%d%s (%p)",
    MSG_process_get_name(MSG_process_self()), MSG_process_self_PID(),
    host,sock->port,sock->meas? " (mode meas)":"",sock);

  return no_error;
}

void gras_trp_sg_socket_close(gras_socket_t sock){
  gras_hostdata_t *hd=(gras_hostdata_t *)MSG_host_get_data(MSG_host_self());
  int cpt;
  
  gras_sg_portrec_t pr;

  XBT_IN1(" (sock=%p)",sock);
   
  if (!sock) return;
  xbt_assert0(hd,"Please run gras_process_init on each process");

  if (sock->data)
    free(sock->data);

  if (sock->incoming && sock->port >= 0) {
    /* server mode socket. Unregister it from 'OS' tables */
    xbt_dynar_foreach(hd->ports, cpt, pr) {
      DEBUG2("Check pr %d of %lu", cpt, xbt_dynar_length(hd->ports));
      if (pr.port == sock->port) {
	xbt_dynar_cursor_rm(hd->ports, &cpt);
	XBT_OUT;
        return;
      }
    }
    WARN2("socket_close called on the unknown incoming socket %p (port=%d)",
	  sock,sock->port);
  }
  XBT_OUT;
}

typedef struct {
  int size;
  void *data;
} sg_task_data_t;

xbt_error_t gras_trp_sg_chunk_send(gras_socket_t sock,
				    const char *data,
				    long int size) {
  m_task_t task=NULL;
  static unsigned int count=0;
  char name[256];
  gras_trp_sg_sock_data_t *sock_data = (gras_trp_sg_sock_data_t *)sock->data;
  sg_task_data_t *task_data;
  
  sprintf(name,"Chunk[%d]",count++);

  task_data=xbt_new(sg_task_data_t,1);
  task_data->data=(void*)xbt_malloc(size);
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

xbt_error_t gras_trp_sg_chunk_recv(gras_socket_t sock,
				    char *data,
				    long int size){
  gras_trp_procdata_t pd=(gras_trp_procdata_t)gras_libdata_get("gras_trp");

  m_task_t task=NULL;
  sg_task_data_t *task_data;
  gras_trp_sg_sock_data_t *sock_data = sock->data;

  XBT_IN;
  DEBUG4("recv chunk on %s ->  %s:%d (size=%ld)",
	 MSG_host_get_name(sock_data->to_host),
	 MSG_host_get_name(MSG_host_self()), sock_data->to_chan, size);
  if (MSG_task_get(&task, (sock->meas ? pd->measChan : pd->chan)) != MSG_OK)
    RAISE0(unknown_error,"Error in MSG_task_get()");
  DEBUG1("Got chuck %s",MSG_task_get_name(task));

  task_data = MSG_task_get_data(task);
  if (size != -1) {	
     if (task_data->size != size)
       RAISE5(mismatch_error,
	      "Got %d bytes when %ld where expected (in %s->%s:%d)",
	      task_data->size, size,
	      MSG_host_get_name(sock_data->to_host),
	      MSG_host_get_name(MSG_host_self()), sock_data->to_chan);
     memcpy(data,task_data->data,size);
  } else {
     /* damn, the size is embeeded at the begining of the chunk */
     int netsize;
     
     memcpy((char*)&netsize,task_data->data,4);
     DEBUG1("netsize embeeded = %d",netsize);

     memcpy(data,task_data->data,netsize+4);
  }
  free(task_data->data);
  free(task_data);

  if (MSG_task_destroy(task) != MSG_OK)
    RAISE0(unknown_error,"Error in MSG_task_destroy()");

  XBT_OUT;
  return no_error;
}

#if 0
/* Data exchange over measurement sockets */
xbt_error_t gras_socket_meas_exchange(gras_socket_t peer,
				      int sender,
				      unsigned int timeout,
				      unsigned long int expSize,
				      unsigned long int msgSize) {
  unsigned int bytesTotal;
  static unsigned int count=0;
  m_task_t task=NULL;
  char name[256];
  gras_trp_sg_sock_data_t *sock_data = (gras_trp_sg_sock_data_t *)peer->data;
  
  gras_trp_procdata_t pd=(gras_trp_procdata_t)gras_libdata_get("gras_trp");
  double startTime;
  
  startTime=gras_os_time(); /* used only in sender mode */

  for(bytesTotal = 0;  bytesTotal < expSize;  bytesTotal += msgSize) {

    if (sender) {
    
      sprintf(name,"meas data[%d]",count++);
      
      task=MSG_task_create(name,0,((double)msgSize)/(1024.0*1024.0),NULL);

      DEBUG5("%f:%s: gras_socket_meas_send(%f %s -> %s) BEGIN",
	     gras_os_time(), MSG_process_get_name(MSG_process_self()),
	     ((double)msgSize)/(1024.0*1024.0),
	     MSG_host_get_name( MSG_host_self()), peer->peer_name);

      if (MSG_task_put(task, sock_data->to_host,sock_data->to_chan) != MSG_OK)
	RAISE0(system_error,"Problem during the MSG_task_put()");
	       
      DEBUG5("%f:%s: gras_socket_meas_send(%f %s -> %s) END",
	     gras_os_time(), MSG_process_get_name(MSG_process_self()),
	     ((double)msgSize)/(1024.0*1024.0),
	     MSG_host_get_name( MSG_host_self()), peer->peer_name);
	           
    } else { /* we are receiver, simulate a select */
      
      task=NULL;
      DEBUG2("%f:%s: gras_socket_meas_recv() BEGIN\n",
	     gras_os_time(), MSG_process_get_name(MSG_process_self()));
      do {
	if (MSG_task_Iprobe((m_channel_t) pd->measChan)) {
	  if (MSG_task_get(&task, (m_channel_t) pd->measChan) != MSG_OK) {
	    fprintf(stderr,"GRAS: Error in MSG_task_get()\n");
	    return unknown_error;
	  }
	  
	  if (MSG_task_destroy(task) != MSG_OK) {
	    fprintf(stderr,"GRAS: Error in MSG_task_destroy()\n");
	    return unknown_error;
	  }
	  
	  DEBUG2("%f:%s: gras_socket_meas_recv() END\n",
		 gras_os_time(), MSG_process_get_name(MSG_process_self()));
	  break;
	} else {
	  MSG_process_sleep(0.0001);
	}
	
      } while (gras_os_time() - startTime < timeout);
      
      if (gras_os_time() - startTime > timeout)
	return timeout_error;
    } /* receiver part */
  } /* foreach msg */

  return no_error;
}
#endif
