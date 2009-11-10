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
  xbt_dict_t sockets; /* all known sockets */
} s_gras_trp_sg_plug_data_t,*gras_trp_sg_plug_data_t;


/***
 *** Code
 ***/

static void gras_trp_sg_exit(gras_trp_plugin_t plug){
  gras_trp_sg_plug_data_t mydata = (gras_trp_sg_plug_data_t) plug->data;
  xbt_dict_free(&(mydata->sockets));
  xbt_free(plug->data);
}
void gras_trp_sg_setup(gras_trp_plugin_t plug)
{
  gras_trp_sg_plug_data_t data = xbt_new(s_gras_trp_sg_plug_data_t, 1);

  plug->data = data;
  data->sockets = xbt_dict_new();

  plug->exit = gras_trp_sg_exit;

  plug->socket_client = gras_trp_sg_socket_client;
  plug->socket_server = gras_trp_sg_socket_server;
  plug->socket_close = gras_trp_sg_socket_close;

  plug->raw_send = gras_trp_sg_chunk_send_raw;
  plug->send = gras_trp_sg_chunk_send;
  plug->raw_recv = plug->recv = gras_trp_sg_chunk_recv;

  plug->flush = NULL;           /* nothing cached */
}

void gras_trp_sg_socket_client(gras_trp_plugin_t self,
                               /* OUT */ gras_socket_t sock) {

  smx_host_t peer;
  gras_trp_sg_sock_data_t *data;
  gras_trp_procdata_t pd= (gras_trp_procdata_t) gras_libdata_by_id(gras_trp_libdata_id);


  if (!(peer = SIMIX_host_get_by_name(sock->peer_name)))
    THROW1(mismatch_error, 0,
           "Can't connect to %s: no such host", sock->peer_name);

  /* make sure this socket will reach someone */
  xbt_dict_t all_sockets = ((gras_trp_sg_plug_data_t)self->data)->sockets;
  char *sock_name=bprintf("%s:%d",sock->peer_name,sock->peer_port);
  gras_socket_t server = xbt_dict_get_or_null(all_sockets,sock_name);
  free(sock_name);

  if (!server)
    THROW2(mismatch_error, 0,
           "can't connect to %s:%d, no process listen on this port",
           sock->peer_name, sock->peer_port);

  if (server->meas && !sock->meas) {
    THROW2(mismatch_error, 0,
           "can't connect to %s:%d in regular mode, the process listen "
           "in measurement mode on this port", sock->peer_name,
           sock->peer_port);
  }
  if (!server->meas && sock->meas) {
    THROW2(mismatch_error, 0,
           "can't connect to %s:%d in measurement mode, the process listen "
           "in regular mode on this port", sock->peer_name, sock->peer_port);
  }
  /* create the socket */
  data = xbt_new0(gras_trp_sg_sock_data_t, 1);
  data->rdv = ((gras_trp_sg_sock_data_t *)server->data)->rdv;
  data->from_process = SIMIX_process_self();

  sock->data = data;
  sock->incoming = 1;

  /* Create a smx comm object about this socket */
  data->ongoing_msg_size = sizeof(s_gras_msg_t);
  smx_comm_t comm = SIMIX_network_irecv(data->rdv,&(data->ongoing_msg),&(data->ongoing_msg_size));
  xbt_dynar_push(pd->comms,&comm);

  DEBUG5("%s (PID %d) connects in %s mode to %s:%d",
         SIMIX_process_get_name(SIMIX_process_self()), gras_os_getpid(),
         sock->meas ? "meas" : "regular", sock->peer_name, sock->peer_port);
}

void gras_trp_sg_socket_server(gras_trp_plugin_t self, gras_socket_t sock) {
  gras_trp_procdata_t pd= (gras_trp_procdata_t) gras_libdata_by_id(gras_trp_libdata_id);
  xbt_dict_t all_sockets = ((gras_trp_sg_plug_data_t)self->data)->sockets;

  /* Make sure that this socket was not opened so far */
  char *sock_name=bprintf("%s:%d",gras_os_myname(),sock->port);
  gras_socket_t old = xbt_dict_get_or_null(all_sockets,sock_name);
  if (old)
    THROW1(mismatch_error, 0,
           "can't listen on address %s: port already in use.",
           sock_name);


  /* Create the data associated to the socket */
  gras_trp_sg_sock_data_t *data = xbt_new0(gras_trp_sg_sock_data_t, 1);
  data->rdv = SIMIX_rdv_create(sock_name);
  data->from_process = SIMIX_process_self();
  SIMIX_rdv_set_data(data->rdv,sock);

  sock->is_master = 1;
  sock->incoming = 1;

  sock->data = data;

  /* Register the socket to the set of sockets known simulation-wide */
  xbt_dict_set(all_sockets,sock_name,sock,NULL); /* FIXME: add a function to raise a warning at simulation end for non-closed sockets */

  /* Create a smx comm object about this socket */
  data->ongoing_msg_size = sizeof(s_gras_msg_t);
  smx_comm_t comm = SIMIX_network_irecv(data->rdv,&(data->ongoing_msg),&(data->ongoing_msg_size));
  INFO2("irecv comm %p onto %p",comm,&(data->ongoing_msg));
  xbt_dynar_push(pd->comms,&comm);

  VERB6("'%s' (%d) ears on %s:%d%s (%p)",
        SIMIX_process_get_name(SIMIX_process_self()), gras_os_getpid(),
        gras_os_myname(), sock->port, sock->meas ? " (mode meas)" : "", sock);
  free(sock_name);
}

void gras_trp_sg_socket_close(gras_socket_t sock) {
  gras_trp_procdata_t pd= (gras_trp_procdata_t) gras_libdata_by_id(gras_trp_libdata_id);
  if (sock->is_master) {
    /* server mode socket. Unregister it from 'OS' tables */
    char *sock_name=bprintf("%s:%d",gras_os_myname(),sock->port);

    xbt_dict_t sockets = ((gras_trp_sg_plug_data_t)sock->plugin->data)->sockets;
    gras_socket_t old = xbt_dict_get_or_null(sockets,sock_name);
    if (!old)
      WARN2("socket_close called on the unknown server socket %p (port=%d)",
            sock, sock->port);
    xbt_dict_remove(sockets,sock_name);
    free(sock_name);

  }

  if (sock->data) {
    SIMIX_rdv_destroy(((gras_trp_sg_sock_data_t *) sock->data)->rdv);
    free(sock->data);
  }

  xbt_dynar_push(pd->sockets_to_close,&sock);
  gras_msg_listener_awake();

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

  gras_trp_sg_sock_data_t *sock_data;
  gras_msg_t msg;               /* message to send */

  sock_data = (gras_trp_sg_sock_data_t *) sock->data;

  xbt_assert0(sock->meas,
              "SG chunk exchange shouldn't be used on non-measurement sockets");

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
  SIMIX_network_send(sock_data->rdv,size,-1.,-1.,&msg,sizeof(msg),(smx_comm_t*)&(msg->comm),msg);
}

int gras_trp_sg_chunk_recv(gras_socket_t sock,
                           char *data, unsigned long int size)
{
  gras_trp_sg_sock_data_t *sock_data;
  gras_msg_t msg_got;
  smx_comm_t comm;

  xbt_assert0(sock->meas,
              "SG chunk exchange shouldn't be used on non-measurement sockets");

  sock_data = (gras_trp_sg_sock_data_t *) sock->data;

  SIMIX_network_recv(sock_data->rdv,-1.,&msg_got,NULL,&comm);

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
  return 0;
}
