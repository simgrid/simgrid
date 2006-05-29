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
#include "gras/Transport/transport_private.h" /* sock->data */

XBT_LOG_EXTERNAL_CATEGORY(gras_msg);
XBT_LOG_DEFAULT_CATEGORY(gras_msg);

void gras_msg_send_ext(gras_socket_t   sock,
		     e_gras_msg_kind_t kind,
		       unsigned long int ID,
		       gras_msgtype_t  msgtype,
		       void           *payload) {

  m_task_t task=NULL;
  gras_trp_sg_sock_data_t *sock_data = (gras_trp_sg_sock_data_t *)sock->data;
  gras_msg_t msg;
  int whole_payload_size=0; /* msg->payload_size is used to memcpy the payload. 
                               This is used to report the load onto the simulator. It also counts the size of pointed stuff */

  xbt_assert1(!gras_socket_is_meas(sock), 
  	      "Asked to send a message on the measurement socket %p", sock);

  msg=xbt_new0(s_gras_msg_t,1);
  msg->type=msgtype;
  msg->ID = ID;
   
  if (kind == e_gras_msg_kind_rpcerror) {
     /* error on remote host, carfull, payload is an exception */
    msg->payl_size=gras_datadesc_size(gras_datadesc_by_name("ex_t"));
    msg->payl=xbt_malloc(msg->payl_size);
    whole_payload_size = gras_datadesc_copy(gras_datadesc_by_name("ex_t"),
					    payload,msg->payl);
  } else if (kind == e_gras_msg_kind_rpcanswer) {
    msg->payl_size=gras_datadesc_size(msgtype->answer_type);
    msg->payl=xbt_malloc(msg->payl_size);
    if (msgtype->answer_type)
      whole_payload_size = gras_datadesc_copy(msgtype->answer_type,
					      payload, msg->payl);
  } else {
    msg->payl_size=gras_datadesc_size(msgtype->ctn_type);
    msg->payl=msg->payl_size?xbt_malloc(msg->payl_size):NULL;
    if (msgtype->ctn_type)
      whole_payload_size = gras_datadesc_copy(msgtype->ctn_type,
					      payload, msg->payl);
  }

  msg->kind = kind;

  task=MSG_task_create(msgtype->name,0,
		       ((double)whole_payload_size),msg);

  if (MSG_task_put(task, sock_data->to_host,sock_data->to_chan) != MSG_OK) 
    THROW0(system_error,0,"Problem during the MSG_task_put");

  VERB5("Sent to %s(%d) a message type '%s' kind '%s' ID %lu",
	MSG_host_get_name(sock_data->to_host),sock_data->to_PID,
	msg->type->name,
	e_gras_msg_kind_names[msg->kind],
	msg->ID);
}
/*
 * receive the next message on the given socket.  
 */
void
gras_msg_recv(gras_socket_t    sock,
	      gras_msg_t       msg) {

  m_task_t task=NULL;
  gras_msg_t msg_got;
  gras_trp_procdata_t pd=(gras_trp_procdata_t)gras_libdata_by_name("gras_trp");

  xbt_assert1(!gras_socket_is_meas(sock), 
  	      "Asked to receive a message on the measurement socket %p", sock);

  xbt_assert0(msg,"msg is an out parameter of gras_msg_recv...");


  if (MSG_task_get(&task, pd->chan) != MSG_OK)
    THROW0(system_error,0,"Error in MSG_task_get()");

  msg_got=MSG_task_get_data(task);


  msg_got->expe= msg->expe;
  memcpy(msg,msg_got,sizeof(s_gras_msg_t));

  free(msg_got);
  if (MSG_task_destroy(task) != MSG_OK)
    THROW0(system_error,0,"Error in MSG_task_destroy()");

  VERB3("Received a message type '%s' kind '%s' ID %lu",// from %s",
	msg->type->name,
	e_gras_msg_kind_names[msg->kind],
	msg->ID);
}
