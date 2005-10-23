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

/** \brief Send the data pointed by \a payload as a message of type
 * \a msgtype to the peer \a sock */
void
gras_msg_send(gras_socket_t   sock,
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

   
  msg->payload_size=gras_datadesc_size(msgtype->ctn_type);
  msg->payload=xbt_malloc(gras_datadesc_size(msgtype->ctn_type));
  if (msgtype->ctn_type)
    whole_payload_size = gras_datadesc_copy(msgtype->ctn_type,payload,msg->payload);

  task=MSG_task_create(msgtype->name,0,
		       ((double)msg->payload_size)/(1024.0*1024.0),msg);

  if (MSG_task_put(task, sock_data->to_host,sock_data->to_chan) != MSG_OK) 
    THROW0(system_error,0,"Problem during the MSG_task_put");

}
/*
 * receive the next message on the given socket.  
 */
void
gras_msg_recv(gras_socket_t    sock,
	      gras_msgtype_t  *msgtype,
	      void           **payload,
	      int             *payload_size) {

  m_task_t task=NULL;
  gras_msg_t msg;
  gras_trp_procdata_t pd=(gras_trp_procdata_t)gras_libdata_by_name("gras_trp");

  xbt_assert1(!gras_socket_is_meas(sock), 
  	      "Asked to receive a message on the measurement socket %p", sock);

  if (MSG_task_get(&task, pd->chan) != MSG_OK)
    THROW0(system_error,0,"Error in MSG_task_get()");

  msg = MSG_task_get_data(task);
  *msgtype = gras_msgtype_by_id(msg->type->code);
  *payload = msg->payload;
  *payload_size = msg->payload_size;

  free(msg);
  if (MSG_task_destroy(task) != MSG_OK)
    THROW0(system_error,0,"Error in MSG_task_destroy()");
}
