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

  double startTime=gras_time();
  gras_procdata_t *pd=gras_procdata_get();
  gras_trp_sg_sock_data_t *sockdata;

  int r_pid, cpt;
  m_process_t remote;
  gras_hostdata_t *remote_hd;
 
  do {
    r_pid = MSG_task_probe_from((m_channel_t) pd->chan);
    if (r_pid >= 0) {
      *dst = pd->sock;
      sockdata = (*dst)->data;

      remote = MSG_process_from_PID(r_pid);
      sockdata->from_PID = r_pid;
      sockdata->to_PID = MSG_process_self_PID();
      sockdata->to_host = MSG_process_get_host(remote);

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
