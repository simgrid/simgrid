/* $Id$ */

/* messaging - Function related to messaging code specific to RL            */

/* Copyright (c) 2003-2005 Martin Quinson. All rights reserved.             */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "gras/Msg/msg_private.h"

#include "gras/DataDesc/datadesc_interface.h"
#include "gras/Transport/transport_interface.h" /* gras_trp_chunk_send/recv */

XBT_LOG_EXTERNAL_CATEGORY(gras_msg);
XBT_LOG_DEFAULT_CATEGORY(gras_msg);

/** \brief Send the data pointed by \a payload as a message of type
 * \a msgtype to the peer \a sock */
void
gras_msg_send(gras_socket_t   sock,
	      gras_msgtype_t  msgtype,
	      void           *payload) {

  static gras_datadesc_type_t string_type=NULL;

  if (!msgtype)
    THROW0(arg_error,0,
	   "Cannot send the NULL message (did msgtype_by_name fail?)");

  if (!string_type) {
    string_type = gras_datadesc_by_name("string");
    xbt_assert(string_type);
  }

  DEBUG3("send '%s' to %s:%d", msgtype->name, 
	 gras_socket_peer_name(sock),gras_socket_peer_port(sock));
  gras_trp_chunk_send(sock, _GRAS_header, 6);

  gras_datadesc_send(sock, string_type,   &msgtype->name);
  if (msgtype->ctn_type)
    gras_datadesc_send(sock, msgtype->ctn_type, payload);
  gras_trp_flush(sock);
}
/*
 * receive the next message on the given socket.  
 */
void
gras_msg_recv(gras_socket_t    sock,
	      gras_msgtype_t  *msgtype,
	      void           **payload,
	      int             *payload_size) {

  xbt_ex_t e;
  static gras_datadesc_type_t string_type=NULL;
  char header[6];
  int cpt;
  int r_arch;
  char *msg_name=NULL;

  xbt_assert1(!gras_socket_is_meas(sock), 
  	      "Asked to receive a message on the measurement socket %p", sock);
  if (!string_type) {
    string_type=gras_datadesc_by_name("string");
    xbt_assert(string_type);
  }
  
  TRY {
    gras_trp_chunk_recv(sock, header, 6);
  } CATCH(e) {
    RETHROW1("Exception caught while trying to get the mesage header on socket %p: %s",
    	     sock);
  }

  for (cpt=0; cpt<4; cpt++)
    if (header[cpt] != _GRAS_header[cpt])
      THROW2(mismatch_error,0,
	     "Incoming bytes do not look like a GRAS message (header='%.4s' not '%.4s')",header,_GRAS_header);
  if (header[4] != _GRAS_header[4]) 
    THROW2(mismatch_error,0,"GRAS protocol mismatch (got %d, use %d)",
	   (int)header[4], (int)_GRAS_header[4]);
  r_arch = (int)header[5];
  DEBUG2("Handle an incoming message using protocol %d (remote is %s)",
	 (int)header[4],gras_datadesc_arch_name(r_arch));

  gras_datadesc_recv(sock, string_type, r_arch, &msg_name);
  TRY {
    *msgtype = (gras_msgtype_t)xbt_set_get_by_name(_gras_msgtype_set,msg_name);
  } CATCH(e) {
    /* FIXME: Survive unknown messages */
    RETHROW1("Exception caught while retrieving the type associated to messages '%s' : %s",
	     msg_name);
  }
  free(msg_name);

  if ((*msgtype)->ctn_type) {
    *payload_size=gras_datadesc_size((*msgtype)->ctn_type);
    xbt_assert2(*payload_size > 0,
		"%s %s",
		"Dynamic array as payload is forbided for now (FIXME?).",
		"Reference to dynamic array is allowed.");
    *payload = xbt_malloc(*payload_size);
    gras_datadesc_recv(sock, (*msgtype)->ctn_type, r_arch, *payload);
  } else {
    *payload = NULL;
    *payload_size = 0;
  }
}
