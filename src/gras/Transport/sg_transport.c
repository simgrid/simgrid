/* $Id$ */

/* sg_transport - SG specific functions for transport                       */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 Martin Quinson.                                       */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras/Transport/transport_private.h"
#include <msg.h>
#include "gras/Virtu/virtu_sg.h"

GRAS_LOG_EXTERNAL_CATEGORY(transport);
GRAS_LOG_DEFAULT_CATEGORY(transport);

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
gras_error_t 
gras_trp_select(double timeout, 
		gras_socket_t **dst) {

  gras_error_t errcode;
  double startTime=gras_os_time();
  gras_procdata_t *pd=gras_procdata_get();
  gras_trp_sg_sock_data_t *sockdata;
  gras_trp_plugin_t *trp;

  gras_socket_t *sock_iter; /* iterating over all sockets */
  int cursor,cpt;

  gras_sg_portrec_t pr;     /* iterating to find the chanel of expeditor */

  int r_pid;
  gras_hostdata_t *remote_hd;

  DEBUG3("select on %s@%s with timeout=%f",
	 MSG_process_get_name(MSG_process_self()),
	 MSG_host_get_name(MSG_host_self()),
	 timeout);
  do {
    r_pid = MSG_task_probe_from((m_channel_t) pd->chan);
    if (r_pid >= 0) {
      /* Try to reuse an already openned socket to that expeditor */
      gras_dynar_foreach(pd->sockets,cursor,sock_iter) {
	DEBUG1("Consider %p as outgoing socket to expeditor",sock_iter);
	sockdata = sock_iter->data;

	if (sock_iter->raw || !sock_iter->outgoing)
	  continue;

	if (sockdata->from_PID == r_pid) {
	  *dst=sock_iter;
	  return no_error;
	}
      }

      /* Socket to expeditor not created yet */
      DEBUG0("Create a socket to the expeditor");

      TRY(gras_trp_plugin_get_by_name("buf",&trp));

      gras_trp_socket_new(1,dst);
      (*dst)->plugin   = trp;

      (*dst)->incoming  = 1;
      (*dst)->outgoing  = 1;
      (*dst)->accepting = 0;
      (*dst)->sd        = -1;

      (*dst)->port      = -1;

      sockdata = gras_new(gras_trp_sg_sock_data_t,1);
      sockdata->from_PID = MSG_process_self_PID();
      sockdata->to_PID   = r_pid;
      sockdata->to_host  = MSG_process_get_host(MSG_process_from_PID(r_pid));
      (*dst)->data = sockdata;
      gras_trp_buf_init_sock(*dst);

      (*dst)->peer_name = strdup(MSG_host_get_name(sockdata->to_host));

      remote_hd=(gras_hostdata_t *)MSG_host_get_data(sockdata->to_host);
      gras_assert0(remote_hd,"Run gras_process_init!!");

      sockdata->to_chan = -1;
      (*dst)->peer_port = -10;
      for (cursor=0; cursor<GRAS_MAX_CHANNEL; cursor++) {
	if (remote_hd->proc[cursor] == r_pid) {
	  sockdata->to_chan = cursor;
	  DEBUG2("Chan %d on %s is for my pal",
		 cursor,(*dst)->peer_name);

	  gras_dynar_foreach(remote_hd->ports, cpt, pr) {
	    if (sockdata->to_chan == pr.tochan) {
	      if (pr.raw) {
		DEBUG0("Damn, it's raw");
		continue;
	      }

	      (*dst)->peer_port = pr.port;
	      DEBUG1("Cool, it points to port %d", pr.port);
	      break;
	    } else {
	      DEBUG2("Wrong port (tochan=%d, looking for %d)\n",
		     pr.tochan,sockdata->to_chan);
	    }
	  }
	  if ((*dst)->peer_port == -10) {
	    /* was raw */
	    sockdata->to_chan = -1;
	  } else {
	    /* found it, don't let it override by raw */
	    break;
	  }
	}
      }
      gras_assert0(sockdata->to_chan != -1,
		   "Got a message from a process without channel");

      return no_error;
    } else {
      /*
      DEBUG2("Select on %s@%s did not find anything yet",
	     MSG_process_get_name(MSG_process_self()),
	     MSG_host_get_name(MSG_host_self()));
      */
      /* MSG_process_sleep(1); */
      MSG_process_sleep(0.001);
    }
  } while (gras_os_time()-startTime < timeout
	   || MSG_task_Iprobe((m_channel_t) pd->chan));

  return timeout_error;

}

  
/* dummy implementations of the functions used in RL mode */

gras_error_t gras_trp_tcp_setup(gras_trp_plugin_t *plug) {
  return mismatch_error;
}
gras_error_t gras_trp_file_setup(gras_trp_plugin_t *plug) {
  return mismatch_error;
}


   
