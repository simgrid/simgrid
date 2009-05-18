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

typedef void* gras_trp_bufdata_;

void gras_msg_send_ext(gras_socket_t   sock,
		e_gras_msg_kind_t kind,
		unsigned long int ID,
		gras_msgtype_t  msgtype,
		void           *payload) {

	smx_action_t act; /* simix action */
	gras_trp_sg_sock_data_t *sock_data;
	gras_hostdata_t *hd;
	gras_trp_procdata_t trp_remote_proc;
	gras_msg_procdata_t msg_remote_proc;
	gras_msg_t msg; /* message to send */
	int whole_payload_size=0; /* msg->payload_size is used to memcpy the payload.
                               This is used to report the load onto the simulator. It also counts the size of pointed stuff */

	sock_data = (gras_trp_sg_sock_data_t *)sock->data;

	hd = (gras_hostdata_t *)SIMIX_host_get_data(SIMIX_host_self());

	xbt_assert1(!gras_socket_is_meas(sock),
			"Asked to send a message on the measurement socket %p", sock);

	/*initialize gras message */
	msg = xbt_new(s_gras_msg_t,1);
	msg->expe = sock;
	msg->kind = kind;
	msg->type = msgtype;
	msg->ID = ID;
	if (kind == e_gras_msg_kind_rpcerror) {
		/* error on remote host, carfull, payload is an exception */
		msg->payl_size=gras_datadesc_size(gras_datadesc_by_name("ex_t"));
		msg->payl=xbt_malloc(msg->payl_size);
		whole_payload_size = gras_datadesc_memcpy(gras_datadesc_by_name("ex_t"),
				payload,msg->payl);
	} else if (kind == e_gras_msg_kind_rpcanswer) {
		msg->payl_size=gras_datadesc_size(msgtype->answer_type);
		if (msg->payl_size)
			msg->payl=xbt_malloc(msg->payl_size);
		else
			msg->payl=NULL;

		if (msgtype->answer_type)
			whole_payload_size = gras_datadesc_memcpy(msgtype->answer_type,
					payload, msg->payl);
	} else {
		msg->payl_size=gras_datadesc_size(msgtype->ctn_type);
		msg->payl=msg->payl_size?xbt_malloc(msg->payl_size):NULL;
		if (msgtype->ctn_type)
			whole_payload_size = gras_datadesc_memcpy(msgtype->ctn_type,
					payload, msg->payl);
	}

	/* put the selectable socket on the queue */
	trp_remote_proc = (gras_trp_procdata_t)
	gras_libdata_by_name_from_remote("gras_trp",sock_data->to_process);

	xbt_queue_push(trp_remote_proc->msg_selectable_sockets,&sock);

	/* put message on msg_queue */
	msg_remote_proc = (gras_msg_procdata_t)
	gras_libdata_by_name_from_remote("gras_msg",sock_data->to_process);
	xbt_fifo_push(msg_remote_proc->msg_to_receive_queue,msg);

	/* wait for the receiver */
	SIMIX_cond_wait(sock_data->cond, sock_data->mutex);

	/* creates simix action and waits its ends, waits in the sender host
     condition*/
	act = SIMIX_action_communicate(SIMIX_host_self(),
			sock_data->to_host,msgtype->name,
			(double)whole_payload_size, -1);
	SIMIX_register_action_to_condition(act,sock_data->cond);

	VERB5("Sending to %s(%s) a message type '%s' kind '%s' ID %lu",
			SIMIX_host_get_name(sock_data->to_host),
			SIMIX_process_get_name(sock_data->to_process),
			msg->type->name,e_gras_msg_kind_names[msg->kind], msg->ID);

	SIMIX_cond_wait(sock_data->cond, sock_data->mutex);
	SIMIX_unregister_action_to_condition(act,sock_data->cond);
	/* error treatmeant (FIXME)*/

	/* cleanup structures */
	SIMIX_action_destroy(act);
	SIMIX_mutex_unlock(sock_data->mutex);

	VERB0("Message sent");

}
/*
 * receive the next message on the given socket.
 */
void
gras_msg_recv(gras_socket_t    sock,
		gras_msg_t       msg) {

	gras_trp_sg_sock_data_t *sock_data;
	gras_trp_sg_sock_data_t *remote_sock_data;
	gras_hostdata_t *remote_hd;
	gras_msg_t msg_got;
	gras_msg_procdata_t msg_procdata =
		(gras_msg_procdata_t)gras_libdata_by_name("gras_msg");

	xbt_assert1(!gras_socket_is_meas(sock),
			"Asked to receive a message on the measurement socket %p", sock);

	xbt_assert0(msg,"msg is an out parameter of gras_msg_recv...");

	sock_data = (gras_trp_sg_sock_data_t *)sock->data;
	remote_sock_data = ((gras_trp_sg_sock_data_t *)sock->data)->to_socket->data;
	DEBUG3("Remote host %s, Remote Port: %d Local port %d",
			SIMIX_host_get_name(sock_data->to_host), sock->peer_port, sock->port);
	remote_hd = (gras_hostdata_t *)SIMIX_host_get_data(sock_data->to_host);

	if (xbt_fifo_size(msg_procdata->msg_to_receive_queue) == 0 ) {
		THROW_IMPOSSIBLE;
	}
	DEBUG1("Size msg_to_receive buffer: %d",
			xbt_fifo_size(msg_procdata->msg_to_receive_queue));
	msg_got = xbt_fifo_shift(msg_procdata->msg_to_receive_queue);

	SIMIX_mutex_lock(remote_sock_data->mutex);
	/* ok, I'm here, you can continuate the communication */
	SIMIX_cond_signal(remote_sock_data->cond);

	/* wait for communication end */
	SIMIX_cond_wait(remote_sock_data->cond,remote_sock_data->mutex);

	msg_got->expe= msg->expe;
	memcpy(msg,msg_got,sizeof(s_gras_msg_t));
	xbt_free(msg_got);
	SIMIX_mutex_unlock(remote_sock_data->mutex);

	VERB3("Received a message type '%s' kind '%s' ID %lu",// from %s",
			msg->type->name,
			e_gras_msg_kind_names[msg->kind],
			msg->ID);
}
