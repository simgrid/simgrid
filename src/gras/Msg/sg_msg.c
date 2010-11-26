/* messaging - Function related to messaging code specific to SG            */

/* Copyright (c) 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"

#include "gras/Virtu/virtu_sg.h"

#include "gras/Msg/msg_private.h"

#include "gras/DataDesc/datadesc_interface.h"
#include "gras/Transport/transport_interface.h" /* gras_trp_chunk_send/recv */
#include "gras/Transport/transport_private.h"   /* sock->data */

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(gras_msg);

typedef void *gras_trp_bufdata_;

gras_msg_t gras_msg_recv_any(void)
{
  gras_trp_procdata_t trp_proc =
      (gras_trp_procdata_t) gras_libdata_by_name("gras_trp");
  gras_msg_t msg;
  /* Build a dynar of all communications I could get something from */
  xbt_dynar_t comms = xbt_dynar_new(sizeof(smx_comm_t), NULL);
  unsigned int cursor = 0;
  int got = 0;
  smx_comm_t comm = NULL;
  gras_socket_t sock = NULL;
  gras_trp_sg_sock_data_t sock_data;
  xbt_dynar_foreach(trp_proc->sockets, cursor, sock) {
    sock_data = (gras_trp_sg_sock_data_t) sock->data;


    DEBUG5
        ("Consider socket %p (data:%p; Here rdv: %p; Remote rdv: %p; Comm %p) to get a message",
         sock, sock_data,
         (sock_data->server ==
          SIMIX_process_self())? sock_data->
         rdv_server : sock_data->rdv_client,
         (sock_data->server ==
          SIMIX_process_self())? sock_data->
         rdv_client : sock_data->rdv_server, sock_data->comm_recv);


    /* If the following assert fails in some valid conditions, we need to
     * change the code downward looking for the socket again.
     *
     * For now it relies on the facts (A) that sockets and comms are aligned
     *                                (B) every sockets has a posted irecv in comms
     *
     * This is not trivial because we need that alignment to hold after the waitany(), so
     * after other processes get scheduled.
     *
     * I cannot think of conditions where they get desynchronized (A violated) as long as
     *    1) only the listener calls that function
     *    2) Nobody but the listener removes sockets from that set (in main listener loop)
     *    3) New sockets are added at the end, and signified ASAP to the listener (by awaking him)
     * The throw bellow ensures that B is never violated without failing out loudly.
     *
     * We cannot search by comparing the comm object pointer that object got
     *    freed by the waiting process (down in smx_network, in
     *    comm_wait_for_completion or comm_cleanup). So, actually, we could
     *    use that pointer since that's a dangling pointer, but no one changes it.
     * I still feel unconfortable with using dangling pointers, even if that would
     *    let the code work even if A and/or B are violated, provided that
     *    (C) the new irecv is never posted before we return from waitany to that function.
     *
     * Another approach, robust to B violation would be to retraverse the socks dynar with
     *    an iterator, incremented only when the socket has a comm. And we've the right socket
     *    when that iterator is equal to "got", the result of waitany. Not needed if B holds.
     */
    xbt_assert1(sock_data->comm_recv,
                "Comm_recv of socket %p is empty; please report that nasty bug",
                sock);
    /* End of paranoia */

    VERB3("Consider receiving messages from on comm_recv %p rdv:%p (other rdv:%p)",
          sock_data->comm_recv,
          (sock_data->server ==
           SIMIX_process_self())? sock_data->
          rdv_server : sock_data->rdv_client,
          (sock_data->server ==
           SIMIX_process_self())? sock_data->
          rdv_client : sock_data->rdv_server);
    xbt_dynar_push(comms, &(sock_data->comm_recv));
  }
  VERB1("Wait on %ld 'sockets'", xbt_dynar_length(comms));
  /* Wait for the end of any of these communications */
  got = SIMIX_network_waitany(comms);

  /* retrieve the message sent in that communication */
  xbt_dynar_get_cpy(comms, got, &(comm));
  msg = SIMIX_communication_get_data(comm);
  sock = xbt_dynar_get_as(trp_proc->sockets, got, gras_socket_t);
  sock_data = (gras_trp_sg_sock_data_t) sock->data;
  VERB3("Got something. Communication %p's over rdv_server=%p, rdv_client=%p",
      comm,sock_data->rdv_server,sock_data->rdv_client);
  SIMIX_communication_destroy(comm);

  /* Reinstall a waiting communication on that rdv */
/*  xbt_dynar_foreach(trp_proc->sockets,cursor,sock) {
    sock_data = (gras_trp_sg_sock_data_t) sock->data;
    if (sock_data->comm_recv && sock_data->comm_recv == comm)
      break;
  }
  */
  sock_data->comm_recv =
      SIMIX_network_irecv(sock_data->rdv_server != NULL ?
                          sock_data->rdv_server
                          : sock_data->rdv_client, NULL, 0);

  return msg;
}


void gras_msg_send_ext(gras_socket_t sock,
                       e_gras_msg_kind_t kind,
                       unsigned long int ID,
                       gras_msgtype_t msgtype, void *payload)
{
  int whole_payload_size = 0;   /* msg->payload_size is used to memcpy the payload.
                                   This is used to report the load onto the simulator. It also counts the size of pointed stuff */
  gras_msg_t msg;               /* message to send */
  smx_comm_t comm;
  gras_trp_sg_sock_data_t sock_data = (gras_trp_sg_sock_data_t) sock->data;

  smx_rdv_t target_rdv =
      (sock_data->server ==
       SIMIX_process_self())? sock_data->
      rdv_client : sock_data->rdv_server;

  /*initialize gras message */
  msg = xbt_new(s_gras_msg_t, 1);
  sock->refcount++;
  msg->expe = sock;
  msg->kind = kind;
  msg->type = msgtype;
  msg->ID = ID;

  VERB4("Send msg %s (%s) to rdv %p sock %p",
      msgtype->name,  e_gras_msg_kind_names[kind], target_rdv, sock);

  if (kind == e_gras_msg_kind_rpcerror) {
    /* error on remote host, careful, payload is an exception */
    msg->payl_size = gras_datadesc_size(gras_datadesc_by_name("ex_t"));
    msg->payl = xbt_malloc(msg->payl_size);
    whole_payload_size =
        gras_datadesc_memcpy(gras_datadesc_by_name("ex_t"), payload,
                             msg->payl);
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

  SIMIX_network_send(target_rdv, whole_payload_size, -1, -1, &msg,
                     sizeof(void *), &comm, msg);

  VERB0("Message sent");

}
