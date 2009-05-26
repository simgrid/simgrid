/* $Id$ */

/* file trp (transport) - send/receive a bunch of bytes in SG realm         */

/* Note that this is only used to debug other parts of GRAS since message   */
/*  exchange in SG realm is implemented directly without mimicing real life */
/*  This would be terribly unefficient.                                     */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"

#include "gras/Msg/msg_private.h"
#include "gras/Transport/transport_private.h"
#include "gras/Virtu/virtu_sg.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras_trp_sg, gras_trp,
                                "SimGrid pseudo-transport");

/***
 *** Prototypes 
 ***/

/* retrieve the port record associated to a numerical port on an host */
static void find_port(gras_hostdata_t * hd, int port,
                      gras_sg_portrec_t * hpd);


void gras_trp_sg_socket_client(gras_trp_plugin_t self,
                               /* OUT */ gras_socket_t sock);
void gras_trp_sg_socket_server(gras_trp_plugin_t self,
                               /* OUT */ gras_socket_t sock);
void gras_trp_sg_socket_close(gras_socket_t sd);

void gras_trp_sg_chunk_send_raw(gras_socket_t sd,
                                const char *data, unsigned long int size);
void gras_trp_sg_chunk_send(gras_socket_t sd,
                            const char *data,
                            unsigned long int size, int stable_ignored);

int gras_trp_sg_chunk_recv(gras_socket_t sd,
                           char *data, unsigned long int size);

/***
 *** Specific plugin part
 ***/
typedef struct {
  int placeholder;              /* nothing plugin specific so far */
} gras_trp_sg_plug_data_t;


/***
 *** Code
 ***/
static void find_port(gras_hostdata_t * hd, int port, gras_sg_portrec_t * hpd)
{
  unsigned int cpt;
  gras_sg_portrec_t pr;

  xbt_assert0(hd, "Please run gras_process_init on each process");

  xbt_dynar_foreach(hd->ports, cpt, pr) {
    if (pr.port == port) {
      memcpy(hpd, &pr, sizeof(gras_sg_portrec_t));
      return;
    }
  }
  THROW1(mismatch_error, 0, "Unable to find any portrec for port #%d", port);
}


void gras_trp_sg_setup(gras_trp_plugin_t plug)
{

  gras_trp_sg_plug_data_t *data = xbt_new(gras_trp_sg_plug_data_t, 1);

  plug->data = data;

  plug->socket_client = gras_trp_sg_socket_client;
  plug->socket_server = gras_trp_sg_socket_server;
  plug->socket_close = gras_trp_sg_socket_close;

  plug->raw_send = gras_trp_sg_chunk_send_raw;
  plug->send = gras_trp_sg_chunk_send;
  plug->raw_recv = plug->recv = gras_trp_sg_chunk_recv;

  plug->flush = NULL;           /* nothing cached */
}

void gras_trp_sg_socket_client(gras_trp_plugin_t self,
                               /* OUT */ gras_socket_t sock)
{
  xbt_ex_t e;

  smx_host_t peer;
  gras_hostdata_t *hd;
  gras_trp_sg_sock_data_t *data;
  gras_sg_portrec_t pr;

  /* make sure this socket will reach someone */
  if (!(peer = SIMIX_host_get_by_name(sock->peer_name)))
    THROW1(mismatch_error, 0,
           "Can't connect to %s: no such host.\n", sock->peer_name);

  if (!(hd = (gras_hostdata_t *) SIMIX_host_get_data(peer)))
    THROW1(mismatch_error, 0,
           "can't connect to %s: no process on this host", sock->peer_name);

  TRY {
    find_port(hd, sock->peer_port, &pr);
  }
  CATCH(e) {
    if (e.category == mismatch_error) {
      xbt_ex_free(e);
      THROW2(mismatch_error, 0,
             "can't connect to %s:%d, no process listen on this port",
             sock->peer_name, sock->peer_port);
    }
    RETHROW;
  }

  if (pr.meas && !sock->meas) {
    THROW2(mismatch_error, 0,
           "can't connect to %s:%d in regular mode, the process listen "
           "in measurement mode on this port", sock->peer_name,
           sock->peer_port);
  }
  if (!pr.meas && sock->meas) {
    THROW2(mismatch_error, 0,
           "can't connect to %s:%d in measurement mode, the process listen "
           "in regular mode on this port", sock->peer_name, sock->peer_port);
  }
  /* create the socket */
  data = xbt_new(gras_trp_sg_sock_data_t, 1);
  data->from_process = SIMIX_process_self();
  data->to_process = pr.process;
  data->to_host = peer;

  /* initialize mutex and condition of the socket */
  data->mutex = SIMIX_mutex_init();
  data->cond = SIMIX_cond_init();
  data->to_socket = pr.socket;

  sock->data = data;
  sock->incoming = 1;

  DEBUG5("%s (PID %d) connects in %s mode to %s:%d",
         SIMIX_process_get_name(SIMIX_process_self()), gras_os_getpid(),
         sock->meas ? "meas" : "regular", sock->peer_name, sock->peer_port);
}

void gras_trp_sg_socket_server(gras_trp_plugin_t self, gras_socket_t sock)
{

  gras_hostdata_t *hd =
    (gras_hostdata_t *) SIMIX_host_get_data(SIMIX_host_self());
  gras_sg_portrec_t pr;
  gras_trp_sg_sock_data_t *data;
  volatile int found;

  const char *host = SIMIX_host_get_name(SIMIX_host_self());

  xbt_ex_t e;

  xbt_assert0(hd, "Please run gras_process_init on each process");

  sock->accepting = 0;          /* no such nuisance in SG */
  found = 0;
  TRY {
    find_port(hd, sock->port, &pr);
    found = 1;
  } CATCH(e) {
    if (e.category == mismatch_error)
      xbt_ex_free(e);
    else
      RETHROW;
  }

  if (found)
    THROW2(mismatch_error, 0,
           "can't listen on address %s:%d: port already in use.",
           host, sock->port);

  pr.port = sock->port;
  pr.meas = sock->meas;
  pr.socket = sock;
  pr.process = SIMIX_process_self();
  xbt_dynar_push(hd->ports, &pr);

  /* Create the socket */
  data = xbt_new(gras_trp_sg_sock_data_t, 1);
  data->from_process = SIMIX_process_self();
  data->to_process = NULL;
  data->to_host = SIMIX_host_self();

  data->cond = SIMIX_cond_init();
  data->mutex = SIMIX_mutex_init();

  sock->data = data;

  VERB6("'%s' (%d) ears on %s:%d%s (%p)",
        SIMIX_process_get_name(SIMIX_process_self()), gras_os_getpid(),
        host, sock->port, sock->meas ? " (mode meas)" : "", sock);

}

void gras_trp_sg_socket_close(gras_socket_t sock)
{
  gras_hostdata_t *hd =
    (gras_hostdata_t *) SIMIX_host_get_data(SIMIX_host_self());
  unsigned int cpt;
  gras_sg_portrec_t pr;

  XBT_IN1(" (sock=%p)", sock);

  if (!sock)
    return;

  xbt_assert0(hd, "Please run gras_process_init on each process");

  if (sock->data) {
    SIMIX_cond_destroy(((gras_trp_sg_sock_data_t *) sock->data)->cond);
    SIMIX_mutex_destroy(((gras_trp_sg_sock_data_t *) sock->data)->mutex);
    free(sock->data);
  }

  if (sock->incoming && !sock->outgoing && sock->port >= 0) {
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
          sock, sock->port);
  }
  XBT_OUT;
}

typedef struct {
  int size;
  void *data;
} sg_task_data_t;

void gras_trp_sg_chunk_send(gras_socket_t sock,
                            const char *data,
                            unsigned long int size, int stable_ignored)
{
  gras_trp_sg_chunk_send_raw(sock, data, size);
}

void gras_trp_sg_chunk_send_raw(gras_socket_t sock,
                                const char *data, unsigned long int size)
{
  char name[256];
  static unsigned int count = 0;

  smx_action_t act;             /* simix action */
  gras_trp_sg_sock_data_t *sock_data;
  gras_trp_procdata_t trp_remote_proc;
  gras_msg_procdata_t msg_remote_proc;
  gras_msg_t msg;               /* message to send */

  sock_data = (gras_trp_sg_sock_data_t *) sock->data;

  xbt_assert0(sock->meas,
              "SG chunk exchange shouldn't be used on non-measurement sockets");

  SIMIX_mutex_lock(sock_data->mutex);
  sprintf(name, "Chunk[%d]", count++);
  /*initialize gras message */
  msg = xbt_new(s_gras_msg_t, 1);
  msg->expe = sock;
  msg->payl_size = size;

  if (data) {
    msg->payl = (void *) xbt_malloc(size);
    memcpy(msg->payl, data, size);
  } else {
    msg->payl = NULL;
  }


  /* put his socket on the selectable socket queue */
  trp_remote_proc = (gras_trp_procdata_t)
    gras_libdata_by_name_from_remote("gras_trp", sock_data->to_process);
  xbt_queue_push(trp_remote_proc->meas_selectable_sockets, &sock);

  /* put message on msg_queue */
  msg_remote_proc = (gras_msg_procdata_t)
    gras_libdata_by_name_from_remote("gras_msg", sock_data->to_process);

  xbt_fifo_push(msg_remote_proc->msg_to_receive_queue_meas, msg);

  /* wait for the receiver */
  SIMIX_cond_wait(sock_data->cond, sock_data->mutex);

  /* creates simix action and waits its ends, waits in the sender host
     condition */
  DEBUG5("send chunk %s from %s to  %s:%d (size=%ld)",
         name, SIMIX_host_get_name(SIMIX_host_self()),
         SIMIX_host_get_name(sock_data->to_host), sock->peer_port, size);

  act = SIMIX_action_communicate(SIMIX_host_self(), sock_data->to_host,
                                 name, size, -1);
  SIMIX_register_action_to_condition(act, sock_data->cond);
  SIMIX_cond_wait(sock_data->cond, sock_data->mutex);
  SIMIX_unregister_action_to_condition(act, sock_data->cond);
  /* error treatmeant (FIXME) */

  /* cleanup structures */
  SIMIX_action_destroy(act);

  SIMIX_mutex_unlock(sock_data->mutex);
}

int gras_trp_sg_chunk_recv(gras_socket_t sock,
                           char *data, unsigned long int size)
{
  gras_trp_sg_sock_data_t *sock_data;
  gras_trp_sg_sock_data_t *remote_sock_data;
  gras_socket_t remote_socket = NULL;
  gras_msg_t msg_got;
  gras_msg_procdata_t msg_procdata =
    (gras_msg_procdata_t) gras_libdata_by_name("gras_msg");
  gras_trp_procdata_t trp_proc =
    (gras_trp_procdata_t) gras_libdata_by_id(gras_trp_libdata_id);

  xbt_assert0(sock->meas,
              "SG chunk exchange shouldn't be used on non-measurement sockets");
  xbt_queue_shift_timed(trp_proc->meas_selectable_sockets,
                        &remote_socket, 60);

  if (remote_socket == NULL) {
    THROW0(timeout_error, 0, "Timeout");
  }

  remote_sock_data = (gras_trp_sg_sock_data_t *) remote_socket->data;
  msg_got = xbt_fifo_shift(msg_procdata->msg_to_receive_queue_meas);

  sock_data = (gras_trp_sg_sock_data_t *) sock->data;

  /* ok, I'm here, you can continue the communication */
  SIMIX_cond_signal(remote_sock_data->cond);

  SIMIX_mutex_lock(remote_sock_data->mutex);
  /* wait for communication end */
  SIMIX_cond_wait(remote_sock_data->cond, remote_sock_data->mutex);

  if (msg_got->payl_size != size)
    THROW5(mismatch_error, 0,
           "Got %d bytes when %ld where expected (in %s->%s:%d)",
           msg_got->payl_size, size,
           SIMIX_host_get_name(sock_data->to_host),
           SIMIX_host_get_name(SIMIX_host_self()), sock->peer_port);

  if (data)
    memcpy(data, msg_got->payl, size);

  if (msg_got->payl)
    xbt_free(msg_got->payl);

  xbt_free(msg_got);
  SIMIX_mutex_unlock(remote_sock_data->mutex);
  return 0;
}
