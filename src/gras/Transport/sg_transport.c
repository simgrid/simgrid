/* $Id$ */

/* sg_transport - SG specific functions for transport                       */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 Martin Quinson.                                       */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "Transport/transport_private.h"
#include <msg.h>

//GRAS_LOG_EXTERNAL_CATEGORY(transport);
//GRAS_LOG_DEFAULT_CATEGORY(transport);

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
 
  do {
    if (MSG_task_Iprobe((m_channel_t) pd->chan)) {
      *dst = pd->sock;

      return no_error;
    } else {
      MSG_process_sleep(0.001);
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
