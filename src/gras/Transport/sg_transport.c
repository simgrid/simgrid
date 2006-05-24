/* $Id$ */

/* sg_transport - SG specific functions for transport                       */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "gras/Transport/transport_private.h"
#include "msg/msg.h"
#include "gras/Virtu/virtu_sg.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(gras_trp);

/**
 * gras_trp_select:
 *
 * Returns the next socket to service having a message awaiting.
 *
 * if timeout<0, we ought to implement the adaptative timeout (FIXME)
 *
 * if timeout=0, do not wait for new message, only handle the ones already there.
 *
 * if timeout>0 and no message there, wait at most that amount of time before giving up.
 */
gras_socket_t gras_trp_select(double timeout) {
  
  gras_socket_t res;
  gras_trp_procdata_t pd = 
    (gras_trp_procdata_t) gras_libdata_by_id(gras_trp_libdata_id);
  gras_trp_sg_sock_data_t *sockdata;
  gras_trp_plugin_t trp;

  gras_socket_t sock_iter; /* iterating over all sockets */
  int cursor,cpt;

  gras_sg_portrec_t pr;     /* iterating to find the chanel of expeditor */

  int r_pid;
  gras_hostdata_t *remote_hd;

  DEBUG3("select on %s@%s with timeout=%f",
	 MSG_process_get_name(MSG_process_self()),
	 MSG_host_get_name(MSG_host_self()),
	 timeout);

  MSG_channel_select_from((m_channel_t) pd->chan, timeout, &r_pid);
  
  if (r_pid < 0) {
    DEBUG0("TIMEOUT");
    THROW0(timeout_error,0,"Timeout");
  }

  /* Ok, got something. Open a socket back to the expeditor */

  /* Try to reuse an already openned socket to that expeditor */
  xbt_dynar_foreach(pd->sockets,cursor,sock_iter) {
    DEBUG1("Consider %p as outgoing socket to expeditor",sock_iter);
    
    if (sock_iter->meas || !sock_iter->outgoing)
      continue;
    
    sockdata = sock_iter->data;
    if (sockdata->to_PID == r_pid) {
      return sock_iter;
    }
  }
  
  /* Socket to expeditor not created yet */
  DEBUG0("Create a socket to the expeditor");
  
  trp = gras_trp_plugin_get_by_name("sg");
  
  gras_trp_socket_new(1,&res);
  res->plugin   = trp;
  
  res->incoming  = 1;
  res->outgoing  = 1;
  res->accepting = 0;
  res->sd        = -1;
  
  res->port      = -1;
  
  sockdata = xbt_new(gras_trp_sg_sock_data_t,1);
  sockdata->from_PID = MSG_process_self_PID();
  sockdata->to_PID   = r_pid;
  sockdata->to_host  = MSG_process_get_host(MSG_process_from_PID(r_pid));
  res->data = sockdata;
  gras_trp_buf_init_sock(res);
  
  res->peer_name = strdup(MSG_host_get_name(sockdata->to_host));
  
  remote_hd=(gras_hostdata_t *)MSG_host_get_data(sockdata->to_host);
  xbt_assert0(remote_hd,"Run gras_process_init!!");
  
  sockdata->to_chan = -1;
  res->peer_port = -10;
  for (cursor=0; cursor<XBT_MAX_CHANNEL; cursor++) {
    if (remote_hd->proc[cursor] == r_pid) {
      sockdata->to_chan = cursor;
      DEBUG2("Chan %d on %s is for my pal",
	     cursor,res->peer_name);
      
      xbt_dynar_foreach(remote_hd->ports, cpt, pr) {
	if (sockdata->to_chan == pr.tochan) {
	  if (pr.meas) {
	    DEBUG0("Damn, it's for measurement");
	    continue;
	  }
	  
	  res->peer_port = pr.port;
	  DEBUG1("Cool, it points to port %d", pr.port);
	  break;
	} else {
	  DEBUG2("Wrong port (tochan=%d, looking for %d)\n",
		 pr.tochan,sockdata->to_chan);
	}
      }
      if (res->peer_port == -10) {
	/* was for measurement */
	sockdata->to_chan = -1;
      } else {
	/* found it, don't let it override by meas */
	break;
      }
    }
  }
  xbt_assert0(sockdata->to_chan != -1,
	      "Got a message from a process without channel");
  
  return res;
}

  
/* dummy implementations of the functions used in RL mode */

void gras_trp_tcp_setup(gras_trp_plugin_t plug) {  THROW0(mismatch_error,0,NULL); }
void gras_trp_file_setup(gras_trp_plugin_t plug){  THROW0(mismatch_error,0,NULL); }
void gras_trp_iov_setup(gras_trp_plugin_t plug) {  THROW0(mismatch_error,0,NULL); }

gras_socket_t gras_trp_buf_init_sock(gras_socket_t sock) { return sock;}
   
