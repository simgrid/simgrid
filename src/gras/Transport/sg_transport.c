/* $Id$ */

/* sg_transport - SG specific functions for transport                       */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 Martin Quinson.                                       */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "Transport/transport_private.h"
#include <msg.h>
#include "Virtu/virtu_sg.h"

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
  double startTime=gras_time();
  gras_procdata_t *pd=gras_procdata_get();
  gras_trp_sg_sock_data_t *sockdata;
  gras_trp_plugin_t *trp;

  gras_socket_t *sock_iter; /* iterating over all sockets */
  int cursor;               /* iterating over all sockets */

  int r_pid, cpt;
  gras_hostdata_t *remote_hd;

  DEBUG1("select with timeout=%d",timeout);
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

      TRY(gras_trp_plugin_get_by_name("sg",&trp));

      TRY(gras_trp_socket_new(1,dst));
      (*dst)->plugin   = trp;

      (*dst)->incoming  = 1;
      (*dst)->outgoing  = 1;
      (*dst)->accepting = 0;
      (*dst)->sd        = -1;

      (*dst)->port      = -1;

      if (!(sockdata = malloc(sizeof(gras_trp_sg_sock_data_t))))
	RAISE_MALLOC;

      sockdata->from_PID = r_pid;
      sockdata->to_PID = MSG_process_self_PID();
      sockdata->to_host = MSG_process_get_host(MSG_process_from_PID(r_pid));
      (*dst)->data = sockdata;

      (*dst)->peer_port = -1;
      (*dst)->peer_name = strdup(MSG_host_get_name(sockdata->to_host));

      remote_hd=(gras_hostdata_t *)MSG_host_get_data(sockdata->to_host);
      gras_assert0(remote_hd,"Run gras_process_init!!");

      sockdata->to_chan = -1;
      for (cpt=0; cpt< GRAS_MAX_CHANNEL; cpt++)
	if (r_pid == remote_hd->proc[cpt])
	  sockdata->to_chan = cpt;

      gras_assert0(sockdata->to_chan>0,
		   "Got a message from a process without channel");

      return no_error;
    } else {
      MSG_process_sleep(0.01);
    }
  } while (gras_time()-startTime < timeout
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
