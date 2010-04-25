/* sg_transport - SG specific functions for transport                       */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "gras/Transport/transport_private.h"
#include "gras/Virtu/virtu_sg.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(gras_trp);

/* check transport_private.h for an explanation of this variable; this just need to be defined to NULL in SG */
gras_socket_t _gras_lastly_selected_socket = NULL;

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
gras_socket_t gras_trp_select(double timeout)
{
  gras_socket_t res;
  gras_trp_procdata_t pd =
    (gras_trp_procdata_t) gras_libdata_by_id(gras_trp_libdata_id);
  gras_trp_sg_sock_data_t *sockdata;
  gras_trp_plugin_t trp;
  gras_socket_t active_socket = NULL;
  gras_trp_sg_sock_data_t *active_socket_data;
  gras_socket_t sock_iter;      /* iterating over all sockets */
  unsigned int cursor;

  DEBUG3("select on %s@%s with timeout=%f",
         SIMIX_process_get_name(SIMIX_process_self()),
         SIMIX_host_get_name(SIMIX_host_self()), timeout);
  if (timeout >= 0) {
    xbt_queue_shift_timed(pd->msg_selectable_sockets,
                          &active_socket, timeout);
  } else {
    xbt_queue_shift(pd->msg_selectable_sockets, &active_socket);
  }

  if (active_socket == NULL) {
    DEBUG0("TIMEOUT");
    THROW0(timeout_error, 0, "Timeout");
  }
  active_socket_data = (gras_trp_sg_sock_data_t *) active_socket->data;

  /* Ok, got something. Open a socket back to the expeditor */

  /* Try to reuse an already openned socket to that expeditor */
  DEBUG1("Open sockets size %lu", xbt_dynar_length(pd->sockets));
  xbt_dynar_foreach(pd->sockets, cursor, sock_iter) {
    gras_trp_sg_sock_data_t *sock_data;
    DEBUG1("Consider %p as outgoing socket to expeditor", sock_iter);

    if (sock_iter->meas || !sock_iter->outgoing)
      continue;
    sock_data = ((gras_trp_sg_sock_data_t *) sock_iter->data);

    if ((sock_data->to_socket == active_socket) &&
        (sock_data->to_host ==
         SIMIX_process_get_host(active_socket_data->from_process))) {
      xbt_dynar_cursor_unlock(pd->sockets);
      return sock_iter;
    }
  }

  /* Socket to expeditor not created yet */
  DEBUG0("Create a socket to the expeditor");

  trp = gras_trp_plugin_get_by_name("sg");

  gras_trp_socket_new(1, &res);
  res->plugin = trp;

  res->incoming = 1;
  res->outgoing = 1;
  res->accepting = 0;
  res->sd = -1;

  res->port = -1;

  /* initialize the ports */
  //res->peer_port = active_socket->port;
  res->port = active_socket->peer_port;

  /* create sockdata */
  sockdata = xbt_new(gras_trp_sg_sock_data_t, 1);
  sockdata->from_process = SIMIX_process_self();
  sockdata->to_process = active_socket_data->from_process;

  res->peer_port = ((gras_trp_procdata_t)
                    gras_libdata_by_name_from_remote("gras_trp",
                                                     sockdata->to_process))->
    myport;
  sockdata->to_socket = active_socket;
  /*update the peer to_socket  variable */
  active_socket_data->to_socket = res;
  sockdata->cond = SIMIX_cond_init();
  sockdata->mutex = SIMIX_mutex_init();

  sockdata->to_host =
    SIMIX_process_get_host(active_socket_data->from_process);

  res->data = sockdata;
  res->peer_name = strdup(SIMIX_host_get_name(sockdata->to_host));

  gras_trp_buf_init_sock(res);

  DEBUG4("Create socket to process:%s(Port %d) from process: %s(Port %d)",
         SIMIX_process_get_name(sockdata->from_process),
         res->peer_port,
         SIMIX_process_get_name(sockdata->to_process), res->port);

  return res;
}


/* dummy implementations of the functions used in RL mode */

void gras_trp_tcp_setup(gras_trp_plugin_t plug)
{
  THROW0(mismatch_error, 0, NULL);
}

void gras_trp_file_setup(gras_trp_plugin_t plug)
{
  THROW0(mismatch_error, 0, NULL);
}

void gras_trp_iov_setup(gras_trp_plugin_t plug)
{
  THROW0(mismatch_error, 0, NULL);
}

gras_socket_t gras_trp_buf_init_sock(gras_socket_t sock)
{
  return sock;
}
