/* file trp (transport) - send/receive a bunch of bytes in SG realm         */

/* Note that this is only used to debug other parts of GRAS since message   */
/*  exchange in SG realm is implemented directly without mimicing real life */
/*  This would be terribly unefficient.                                     */

/* Copyright (c) 2004, 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"

#include "simix/simix.h"
#include "gras/Msg/msg_private.h"
#include "gras/Transport/transport_private.h"
#include "gras/Virtu/virtu_sg.h"
#include "xbt/xbt_socket_private.h" /* FIXME */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras_trp_sg, gras_trp,
                                "SimGrid pseudo-transport");

/***
 *** Prototypes 
 ***/

/* retrieve the port record associated to a numerical port on an host */
static gras_sg_portrec_t find_port(gras_hostdata_t * hd, int port);

void gras_trp_sg_socket_client(xbt_trp_plugin_t self,
                               const char*host,
                               int port,
                               /* OUT */ xbt_socket_t sock);
void gras_trp_sg_socket_server(xbt_trp_plugin_t self,
                               int port,
                               /* OUT */ xbt_socket_t sock);
void gras_trp_sg_socket_close(xbt_socket_t sd);

void gras_trp_sg_chunk_send_raw(xbt_socket_t sd,
                                const char *data, unsigned long int size);
void gras_trp_sg_chunk_send(xbt_socket_t sd,
                            const char *data,
                            unsigned long int size, int stable_ignored);

int gras_trp_sg_chunk_recv(xbt_socket_t sd,
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
static gras_sg_portrec_t find_port(gras_hostdata_t * hd, int port)
{
  unsigned int cpt;
  gras_sg_portrec_t pr;

  xbt_assert(hd, "Please run gras_process_init on each process");

  xbt_dynar_foreach(hd->ports, cpt, pr) {
    if (pr->port == port)
      return pr;
  }
  return NULL;
}

/***
 *** Info about who's speaking
 ***/
static int gras_trp_sg_my_port(xbt_socket_t s)
{
  gras_trp_sg_sock_data_t sockdata = xbt_socket_get_data(s);
  if (gras_socket_im_the_server(s))
    return sockdata->server_port;
  else
    return sockdata->client_port;
}
static int gras_trp_sg_peer_port(xbt_socket_t s)
{
  gras_trp_sg_sock_data_t sockdata = xbt_socket_get_data(s);
  if (gras_socket_im_the_server(s))
    return sockdata->client_port;
  else
    return sockdata->server_port;
}

static const char* gras_trp_sg_peer_name(xbt_socket_t s)
{
  gras_trp_sg_sock_data_t sockdata = xbt_socket_get_data(s);
  if (gras_socket_im_the_server(s))
    return SIMIX_host_get_name(simcall_process_get_host(sockdata->client));
  else {
    return SIMIX_host_get_name(simcall_process_get_host(sockdata->server));
  }
}
static const char* gras_trp_sg_peer_proc(xbt_socket_t s) {
  THROW_UNIMPLEMENTED;
}
static void gras_trp_sg_peer_proc_set(xbt_socket_t s,char *name) {
  THROW_UNIMPLEMENTED;
}

void gras_trp_sg_setup(xbt_trp_plugin_t plug)
{
  plug->my_port = gras_trp_sg_my_port;
  plug->peer_port = gras_trp_sg_peer_port;
  plug->peer_name = gras_trp_sg_peer_name;
  plug->peer_proc = gras_trp_sg_peer_proc;
  plug->peer_proc_set = gras_trp_sg_peer_proc_set;

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

void gras_trp_sg_socket_client(xbt_trp_plugin_t self,
                               const char*host,
                               int port,
                               /* OUT */ xbt_socket_t sock)
{

  smx_host_t peer;
  gras_hostdata_t *hd;
  gras_trp_sg_sock_data_t data;
  gras_sg_portrec_t pr;

  /* make sure this socket will reach someone */
  if (!(peer = simcall_host_get_by_name(host)))
    THROWF(mismatch_error, 0,
           "Can't connect to %s: no such host.\n", host);

  if (!(hd = (gras_hostdata_t *) simcall_host_get_data(peer)))
    THROWF(mismatch_error, 0,
           "can't connect to %s: no process on this host",
           host);

  pr = find_port(hd, port);

  if (pr == NULL) {
    THROWF(mismatch_error, 0,
           "can't connect to %s:%d, no process listen on this port",
           host, port);
  }

  /* Ensure that the listener is expecting the kind of stuff we want to send */
  if (pr->meas && !sock->meas) {
    THROWF(mismatch_error, 0,
           "can't connect to %s:%d in regular mode, the process listen "
           "in measurement mode on this port", host,
           port);
  }
  if (!pr->meas && sock->meas) {
    THROWF(mismatch_error, 0,
           "can't connect to %s:%d in measurement mode, the process listen "
           "in regular mode on this port", host,
           port);
  }

  /* create simulation data of the socket */
  data = xbt_new0(s_gras_trp_sg_sock_data_t, 1);
  data->client = SIMIX_process_self();
  data->server = pr->server;
  data->server_port = port;
  data->client_port = gras_os_myport();

  /* initialize synchronization stuff on the socket */
  data->rdv_server = pr->rdv;
  data->rdv_client = simcall_rdv_create(NULL);
  data->comm_recv = simcall_comm_irecv(data->rdv_client, &data->msg, NULL, NULL, NULL);

  /* connect that simulation data to the socket */
  sock->data = data;
  sock->incoming = 1;

  XBT_DEBUG("%s (PID %d) connects in %s mode to %s:%d (rdv_ser:%p, rdv_cli:%p, comm:%p)",
         simcall_process_get_name(SIMIX_process_self()), gras_os_getpid(),
         sock->meas ? "meas" : "regular", host, port,
         data->rdv_server,data->rdv_client,data->comm_recv);
}

void gras_trp_sg_socket_server(xbt_trp_plugin_t self, int port, xbt_socket_t sock)
{

  gras_hostdata_t *hd =
      (gras_hostdata_t *) SIMIX_host_self_get_data();
  gras_sg_portrec_t pr;
  gras_trp_sg_sock_data_t data;

  xbt_assert(hd, "Please run gras_process_init on each process");

  sock->accepting = 1;

  /* Check whether a server is already listening on that port or not */
  pr = find_port(hd, port);

  if (pr)
    THROWF(mismatch_error, 0,
           "can't listen on address %s:%d: port already in use.",
           SIMIX_host_self_get_name(), port);

  /* This port is free, let's take it */
  pr = xbt_new(s_gras_sg_portrec_t, 1);
  pr->port = port;
  pr->meas = sock->meas;
  pr->server = SIMIX_process_self();
  xbt_dynar_push(hd->ports, &pr);
  pr->rdv = simcall_rdv_create(NULL);

  /* Create the socket */
  data = xbt_new0(s_gras_trp_sg_sock_data_t, 1);
  data->server = SIMIX_process_self();
  data->server_port = port;
  data->client = NULL;
  data->rdv_server = pr->rdv;
  data->rdv_client = NULL;
  data->comm_recv = simcall_comm_irecv(pr->rdv, &data->msg, NULL, NULL, NULL);

  sock->data = data;

  XBT_VERB
      ("'%s' (%d) ears on %s:%d%s (%p; data:%p); Here rdv: %p; Remote rdv: %p; Comm %p",
       simcall_process_get_name(SIMIX_process_self()), gras_os_getpid(),
       SIMIX_host_self_get_name(), port,
       sock->meas ? " (mode meas)" : "", sock, data,
       (data->server ==
        SIMIX_process_self())? data->rdv_server : data->rdv_client,
       (data->server ==
        SIMIX_process_self())? data->rdv_client : data->rdv_server,
       data->comm_recv);

}

void gras_trp_sg_socket_close(xbt_socket_t sock)
{
  gras_hostdata_t *hd =
      (gras_hostdata_t *) SIMIX_host_self_get_data();
  unsigned int cpt;
  gras_sg_portrec_t pr;

  XBT_IN(" (sock=%p)", sock);

  if (!sock)
    return;

  xbt_assert(hd, "Please run gras_process_init on each process");

  gras_trp_sg_sock_data_t sockdata = sock->data;

  if (sock->incoming && !sock->outgoing && sockdata->server_port >= 0) {
    /* server mode socket. Unregister it from 'OS' tables */
    xbt_dynar_foreach(hd->ports, cpt, pr) {
      XBT_DEBUG("Check pr %d of %lu", cpt, xbt_dynar_length(hd->ports));
      if (pr->port == sockdata->server_port) {
        xbt_dynar_cursor_rm(hd->ports, &cpt);
        XBT_OUT();
        return;
      }
    }
    XBT_WARN
        ("socket_close called on the unknown incoming socket %p (port=%d)",
         sock, sockdata->server_port);
  }
  /* FIXME: kill the rdv point if receiver side */
  free(sock->data);
  XBT_OUT();
}

typedef struct {
  int size;
  void *data;
} sg_task_data_t;

void gras_trp_sg_chunk_send(xbt_socket_t sock,
                            const char *data,
                            unsigned long int size, int stable_ignored)
{
  gras_trp_sg_chunk_send_raw(sock, data, size);
}

void gras_trp_sg_chunk_send_raw(xbt_socket_t sock,
                                const char *data, unsigned long int size)
{
#ifdef KILLME
  char name[256];
  static unsigned int count = 0;

  smx_action_t act;             /* simix action */
  gras_trp_procdata_t trp_remote_proc;
  gras_msg_procdata_t msg_remote_proc;
  gras_msg_t msg;               /* message to send */

  //gras_trp_sg_sock_data_t sock_data = (gras_trp_sg_sock_data_t) sock->data;
  xbt_assert(sock->meas,
              "SG chunk exchange shouldn't be used on non-measurement sockets");


  /* creates simix action and waits its ends, waits in the sender host
     condition */
  /*
  if (XBT_LOG_ISENABLED(gras_trp_sg, xbt_log_priority_debug)) {
    smx_process_t remote_dude =
        (sock_data->server ==
         SIMIX_process_self())? (sock_data->client) : (sock_data->server);
    smx_host_t remote_host = simcall_process_get_host(remote_dude);
  }
  */
  //SIMIX_network_send(sock_data->rdv,size,1,-1,NULL,0,NULL,NULL);
#endif
  THROW_UNIMPLEMENTED;
}

int gras_trp_sg_chunk_recv(xbt_socket_t sock,
                           char *data, unsigned long int size)
{
  //gras_trp_sg_sock_data_t *sock_data =
  //    (gras_trp_sg_sock_data_t *) sock->data;

  //simcall_comm_recv(sock_data->rdv,-1,NULL,0,NULL);
  THROW_UNIMPLEMENTED;
#ifdef KILLME
  gras_trp_sg_sock_data_t *remote_sock_data;
  xbt_socket_t remote_socket = NULL;
  gras_msg_t msg_got;
  gras_msg_procdata_t msg_procdata =
      (gras_msg_procdata_t) gras_libdata_by_name("gras_msg");
  gras_trp_procdata_t trp_proc =
      (gras_trp_procdata_t) gras_libdata_by_id(gras_trp_libdata_id);

  xbt_assert(sock->meas,
              "SG chunk exchange shouldn't be used on non-measurement sockets");
  xbt_queue_shift_timed(trp_proc->meas_selectable_sockets,
                        &remote_socket, 60);

  if (remote_socket == NULL) {
    THROWF(timeout_error, 0, "Timeout");
  }

  remote_sock_data = (gras_trp_sg_sock_data_t *) remote_socket->data;
  msg_got = xbt_fifo_shift(msg_procdata->msg_to_receive_queue_meas);

  sock_data = (gras_trp_sg_sock_data_t *) sock->data;

  /* ok, I'm here, you can continue the communication */
  simcall_cond_signal(remote_sock_data->cond);

  simcall_mutex_lock(remote_sock_data->mutex);
  /* wait for communication end */
  simcall_cond_wait(remote_sock_data->cond, remote_sock_data->mutex);

  if (msg_got->payl_size != size)
    THROWF(mismatch_error, 0,
           "Got %d bytes when %ld where expected (in %s->%s:%d)",
           msg_got->payl_size, size,
           simcall_host_get_name(sock_data->to_host),
           SIMIX_host_self_get_name(), sock->peer_port);

  if (data)
    memcpy(data, msg_got->payl, size);

  xbt_free(msg_got->payl);

  xbt_free(msg_got);
  simcall_mutex_unlock(remote_sock_data->mutex);
#endif
  return 0;
}
