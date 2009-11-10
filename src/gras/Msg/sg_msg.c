/* $Id$ */

/* messaging - Function related to messaging code specific to SG            */

/* Copyright (c) 2003-2005 Martin Quinson. All rights reserved.             */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"

#include "gras/Virtu/virtu_sg.h"

#include "gras/Msg/msg_private.h"

#include "gras/DataDesc/datadesc_interface.h"
#include "gras/Transport/transport_interface.h" /* gras_trp_chunk_send/recv */
#include "gras/Transport/transport_private.h"   /* sock->data */

XBT_LOG_EXTERNAL_CATEGORY(gras_msg);
XBT_LOG_DEFAULT_CATEGORY(gras_msg);

typedef void *gras_trp_bufdata_;

void gras_msg_send_ext(gras_socket_t sock,
                       e_gras_msg_kind_t kind,
                       unsigned long int ID,
                       gras_msgtype_t msgtype, void *payload)
{
  gras_trp_sg_sock_data_t *sock_data;
  gras_msg_t msg;               /* message to send */
  int whole_payload_size = 0;   /* msg->payload_size is used to memcpy the payload.
                                   This is used to report the load onto the simulator. It also counts the size of pointed stuff */

  sock_data = (gras_trp_sg_sock_data_t *) sock->data;

  xbt_assert1(!gras_socket_is_meas(sock),
              "Asked to send a message on the measurement socket %p", sock);

  /*initialize gras message */
  msg = xbt_new(s_gras_msg_t, 1);
  msg->expe = sock;
  msg->kind = kind;
  msg->type = msgtype;
  msg->ID = ID;
  if (kind == e_gras_msg_kind_rpcerror) {
    /* error on remote host, careful, payload is an exception */
    msg->payl_size = gras_datadesc_size(gras_datadesc_by_name("ex_t"));
    msg->payl = xbt_malloc(msg->payl_size);
    whole_payload_size = gras_datadesc_memcpy(gras_datadesc_by_name("ex_t"),
                                              payload, msg->payl);
  } else if (kind == e_gras_msg_kind_rpcanswer) {
    msg->payl_size = gras_datadesc_size(msgtype->answer_type);
    if (msg->payl_size)
      msg->payl = xbt_malloc(msg->payl_size);
    else
      msg->payl = NULL;

    if (msgtype->answer_type)
      whole_payload_size = gras_datadesc_memcpy(msgtype->answer_type,
                                                payload, msg->payl);
  } else {
    msg->payl_size = gras_datadesc_size(msgtype->ctn_type);
    msg->payl = msg->payl_size ? xbt_malloc(msg->payl_size) : NULL;
    if (msgtype->ctn_type)
      whole_payload_size = gras_datadesc_memcpy(msgtype->ctn_type,
                                                payload, msg->payl);
  }



  VERB5("Sending to %s(%s) a message type '%s' kind '%s' ID %lu",
        sock->peer_name,sock->peer_proc,
        msg->type->name, e_gras_msg_kind_names[msg->kind], msg->ID);
  SIMIX_network_send(sock_data->rdv,whole_payload_size,-1.,-1.,msg,sizeof(s_gras_msg_t),(smx_comm_t*)&(msg->comm),&msg);

  VERB0("Message sent");
}

/*
 * receive the next message on the given socket.
 */
void gras_msg_recv(gras_socket_t sock, gras_msg_t msg) {
  gras_trp_procdata_t pd =
    (gras_trp_procdata_t) gras_libdata_by_id(gras_trp_libdata_id);
  gras_trp_sg_sock_data_t *sock_data;

  xbt_assert1(!gras_socket_is_meas(sock),
              "Asked to receive a message on the measurement socket %p",
              sock);

  xbt_assert0(msg, "msg is an out parameter of gras_msg_recv...");

  sock_data = (gras_trp_sg_sock_data_t *) sock->data;

  /* The message was already received while emulating the select, so simply copy it here */
  memcpy(msg,&(sock_data->ongoing_msg),sizeof(s_gras_msg_t));
  msg->expe = sock;
  VERB1("Using %p as a msg",&(sock_data->ongoing_msg));
  VERB5("Received a message type '%s' kind '%s' ID %lu from %s(%s)",
        msg->type->name, e_gras_msg_kind_names[msg->kind], msg->ID,
        sock->peer_name,sock->peer_proc);

  /* Recreate another comm object to replace the one which just terminated */
  int rank = xbt_dynar_search(pd->sockets,&sock);
  xbt_assert0(rank>=0,"Socket not found in my array");
  sock_data->ongoing_msg_size = sizeof(s_gras_msg_t);
  smx_comm_t comm = SIMIX_network_irecv(sock_data->rdv,&(sock_data->ongoing_msg),&(sock_data->ongoing_msg_size));
  xbt_dynar_set(pd->comms,rank,&comm);

#if 0 /* KILLME */
  SIMIX_network_recv(sock_data->rdv,-1.,&msg_got,NULL,&comm);


  remote_sock_data =
    ((gras_trp_sg_sock_data_t *) sock->data)->to_socket->data;
  remote_hd = (gras_hostdata_t *) SIMIX_host_get_data(sock_data->to_host);

  if (xbt_fifo_size(msg_procdata->msg_to_receive_queue) == 0) {
    THROW_IMPOSSIBLE;
  }
  DEBUG1("Size msg_to_receive buffer: %d",
         xbt_fifo_size(msg_procdata->msg_to_receive_queue));
  msg_got = xbt_fifo_shift(msg_procdata->msg_to_receive_queue);

  SIMIX_mutex_lock(remote_sock_data->mutex);
  /* ok, I'm here, you can continuate the communication */
  SIMIX_cond_signal(remote_sock_data->cond);

  /* wait for communication end */
  INFO2("Wait communication (from %s) termination on %p",sock->peer_name,sock_data->cond);

  SIMIX_cond_wait(remote_sock_data->cond, remote_sock_data->mutex);

  msg_got->expe = msg->expe;
  memcpy(msg, msg_got, sizeof(s_gras_msg_t));
  xbt_free(msg_got);
  SIMIX_mutex_unlock(remote_sock_data->mutex);
#endif
}
