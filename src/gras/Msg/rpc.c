/* $Id$ */

/* rpc - RPC implementation on top of GRAS messages                         */

/* Copyright (c) 2005 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras/Msg/msg_private.h"
                
//XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras_rpc,gras_msg,"RPCing");

xbt_set_t _gras_rpctype_set = NULL;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras_rpc,gras_msg,"RPC mecanism");


/** @brief declare a new versionned RPC type of the given name and payloads
 *
 * @param name: name as it should be used for logging messages (must be uniq)
 * @param version: something like versionning symbol
 * @param payload: datadescription of the payload
 *
 * Registers a new RPC message to the GRAS mechanism. RPC are constituted of a pair 
 * of messages. 
 */
void
gras_msgtype_declare_rpc(const char           *name,
			 gras_datadesc_type_t  payload_request,
			 gras_datadesc_type_t  payload_answer) {

  gras_msgtype_declare_ext(name, 0, 
			   e_gras_msg_kind_rpccall, 
			   payload_request, payload_answer);

}

/** @brief declare a new versionned RPC type of the given name and payloads
 *
 * @param name: name as it should be used for logging messages (must be uniq)
 * @param version: something like versionning symbol
 * @param payload: datadescription of the payload
 *
 * Registers a new RPC message to the GRAS mechanism. RPC are constituted of a pair 
 * of messages. 
 *
 * Use this version instead of gras_rpctype_declare when you change the
 * semantic or syntax of a message and want your programs to be able to deal
 * with both versions. Internally, each will be handled as an independent
 * message type, so you can register differents for each of them.
 */
void
gras_msgtype_declare_rpc_v(const char           *name,
			   short int             version,
			   gras_datadesc_type_t  payload_request,
			   gras_datadesc_type_t  payload_answer) {

  gras_msgtype_declare_ext(name, version, 
			   e_gras_msg_kind_rpccall, 
			   payload_request, payload_answer);

}

static unsigned long int last_msg_ID = 0;

static int msgfilter_rpcID(gras_msg_t msg, void* ctx) {
  unsigned long int ID= *(unsigned long int*)ctx;
  int res = msg->ID == ID && 
    (msg->kind == e_gras_msg_kind_rpcanswer || msg->kind == e_gras_msg_kind_rpcerror);

  DEBUG5("Filter a message of ID %lu, type '%s' and kind '%s'. Waiting for ID=%lu. %s",
	 msg->ID,msg->type->name,e_gras_msg_kind_names[msg->kind],ID,
	 res?"take it": "reject");
  return res;
}

/** @brief Conduct a RPC call
 *
 */
void gras_msg_rpccall(gras_socket_t server,
		      double timeOut,
		      gras_msgtype_t msgtype,
		      void *request, void *answer) {

  unsigned long int msg_ID = last_msg_ID++;
  s_gras_msg_t received;

  DEBUG2("Send a RPC of type '%s' (ID=%lu)",msgtype->name,msg_ID);

  gras_msg_send_ext(server, e_gras_msg_kind_rpccall, msg_ID, msgtype, request);
  gras_msg_wait_ext(timeOut,
		    msgtype, NULL, msgfilter_rpcID, &msg_ID,
		    &received);
  if (received.kind == e_gras_msg_kind_rpcerror) {
    /* Damn. Got an exception. Extract it and revive it */
    xbt_ex_t e;
    VERB0("Raise a remote exception");
    memcpy(&e,received.payl,received.payl_size);
    free(received.payl);
    memcpy((void*)&(__xbt_ex_ctx()->ctx_ex),&e,sizeof(xbt_ex_t));
    DO_THROW(__xbt_ex_ctx()->ctx_ex);

  }
  memcpy(answer,received.payl,received.payl_size);
  free(received.payl);
}


/** @brief Return the result of a RPC call
 *
 * It done before the actual return of the callback so that the callback can do
 * some cleanups before leaving.
 */

void gras_msg_rpcreturn(double timeOut,gras_msg_cb_ctx_t ctx,void *answer) {
  gras_msg_send_ext(ctx->expeditor, e_gras_msg_kind_rpcanswer, ctx->ID, ctx->msgtype, answer);
}

