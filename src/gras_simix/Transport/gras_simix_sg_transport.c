/* $Id$ */

/* sg_transport - SG specific functions for transport                       */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "gras_simix/Transport/gras_simix_transport_private.h"
//#include "msg/msg.h"
#include "gras_simix/Virtu/gras_simix_virtu_sg.h"

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
gras_socket_t gras_trp_select(double timeout) {
	gras_socket_t res;
	gras_trp_procdata_t pd = (gras_trp_procdata_t) gras_libdata_by_id(gras_trp_libdata_id);
	gras_trp_sg_sock_data_t *sockdata;
	gras_trp_plugin_t trp;
	gras_socket_t active_socket;
	gras_socket_t sock_iter; /* iterating over all sockets */
	int cursor;
//	int i;

	gras_hostdata_t *remote_hd;
//	gras_hostdata_t *local_hd;
	
	DEBUG0("Trying to get the lock pd, trp_select");
	SIMIX_mutex_lock(pd->mutex);
	DEBUG3("select on %s@%s with timeout=%f",
			SIMIX_process_get_name(SIMIX_process_self()),
			SIMIX_host_get_name(SIMIX_host_self()),
			timeout);

	if (xbt_fifo_size(pd->active_socket) == 0) {
	/* message didn't arrive yet, wait */
		SIMIX_cond_wait_timeout(pd->cond,pd->mutex,timeout);
	}

	if (xbt_fifo_size(pd->active_socket) == 0) {
		DEBUG0("TIMEOUT");
		SIMIX_mutex_unlock(pd->mutex);
		THROW0(timeout_error,0,"Timeout");
	}
	active_socket = xbt_fifo_shift(pd->active_socket);
	
	/* Ok, got something. Open a socket back to the expeditor */

	/* Try to reuse an already openned socket to that expeditor */
	DEBUG1("Open sockets size %lu",xbt_dynar_length(pd->sockets));
	xbt_dynar_foreach(pd->sockets,cursor,sock_iter) {
		DEBUG1("Consider %p as outgoing socket to expeditor",sock_iter);

		if (sock_iter->meas || !sock_iter->outgoing)
			continue;
		if ((sock_iter->peer_port == active_socket->port) && 
				(((gras_trp_sg_sock_data_t*)sock_iter->data)->to_host == SIMIX_process_get_host(((gras_trp_sg_sock_data_t*)active_socket->data)->from_process))) {
			SIMIX_mutex_unlock(pd->mutex);
			return sock_iter;
		}
	}

	/* Socket to expeditor not created yet */
	DEBUG0("Create a socket to the expeditor");

	trp = gras_trp_plugin_get_by_name("sg");

	gras_trp_socket_new(1,&res);
	res->plugin   = trp;

	res->incoming  = 1;
	res->outgoing  = 1;
	res->accepting = 0;
	res->sd        = -1;

	res->port = -1;

	sockdata = xbt_new(gras_trp_sg_sock_data_t,1);
	sockdata->from_process = SIMIX_process_self();
	sockdata->to_process   = ((gras_trp_sg_sock_data_t*)(active_socket->data))->from_process;
	
	DEBUG2("Create socket to process:%s from process: %s",SIMIX_process_get_name(sockdata->from_process),SIMIX_process_get_name(sockdata->to_process));
	/* complicated*/
	sockdata->to_host  = SIMIX_process_get_host(((gras_trp_sg_sock_data_t*)(active_socket->data))->from_process);

	res->data = sockdata;
	gras_trp_buf_init_sock(res);

	res->peer_name = strdup(SIMIX_host_get_name(sockdata->to_host));

	remote_hd=(gras_hostdata_t *)SIMIX_host_get_data(sockdata->to_host);
	xbt_assert0(remote_hd,"Run gras_process_init!!");

	res->peer_port = active_socket->port;

	res->port = active_socket->peer_port;
	/* search for a free port on the host */
	/*
	local_hd = (gras_hostdata_t *)SIMIX_host_get_data(SIMIX_host_self());
	for (i=1;i<65536;i++) {
		if (local_hd->cond_port[i] == NULL)
			break;
	}
	if (i == 65536) {
		SIMIX_mutex_unlock(pd->mutex);
		THROW0(system_error,0,"No port free");
	}
	res->port = i;*/
	/*initialize the cond and mutex */
	/*
	local_hd->cond_port[i] = SIMIX_cond_init();
	local_hd->mutex_port[i] = SIMIX_mutex_init();
	*/
	/* update remote peer_port to talk through the new port */
	active_socket->peer_port = res->port;



	DEBUG2("New socket: Peer port %d Local port %d", res->peer_port, res->port);
	SIMIX_mutex_unlock(pd->mutex);
	return res;
}

  
/* dummy implementations of the functions used in RL mode */

void gras_trp_tcp_setup(gras_trp_plugin_t plug) {  THROW0(mismatch_error,0,NULL); }
void gras_trp_file_setup(gras_trp_plugin_t plug){  THROW0(mismatch_error,0,NULL); }
void gras_trp_iov_setup(gras_trp_plugin_t plug) {  THROW0(mismatch_error,0,NULL); }

gras_socket_t gras_trp_buf_init_sock(gras_socket_t sock) { return sock;}
   
