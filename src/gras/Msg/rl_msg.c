/* $Id$ */

/* messaging - Function related to messaging code specific to RL            */

/* Copyright (c) 2003-2005 Martin Quinson. All rights reserved.             */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "gras/Msg/msg_private.h"

#include "gras/DataDesc/datadesc_interface.h"
#include "gras/Transport/transport_interface.h" /* gras_trp_send/recv */

XBT_LOG_EXTERNAL_CATEGORY(gras_msg);
XBT_LOG_DEFAULT_CATEGORY(gras_msg);

void gras_msg_send_ext(gras_socket_t   sock,
		     e_gras_msg_kind_t kind,
		       unsigned long int ID,
		       gras_msgtype_t  msgtype,
		       void           *payload) {

  static gras_datadesc_type_t string_type=NULL;
  static gras_datadesc_type_t ulong_type=NULL;
  char c_kind=(char)kind;
  
  xbt_assert0(msgtype,"Cannot send the NULL message");

  if (!string_type) {
    string_type = gras_datadesc_by_name("string");
    xbt_assert(string_type);
  }
  if (!ulong_type) {
    ulong_type = gras_datadesc_by_name("unsigned long int");
    xbt_assert(ulong_type);
  }

  DEBUG3("send '%s' to %s:%d", msgtype->name, 
	 gras_socket_peer_name(sock),gras_socket_peer_port(sock));
  gras_trp_send(sock, _GRAS_header, 6, 1 /* stable */);
  gras_trp_send(sock, &c_kind,      1, 1 /* stable */);
  switch (kind) {
   case e_gras_msg_kind_oneway: 
     break;
     
   case e_gras_msg_kind_rpccall:
   case e_gras_msg_kind_rpcanswer:
   case e_gras_msg_kind_rpcerror:
     gras_datadesc_send(sock,ulong_type,&ID);
     break;

   default:
     THROW1(unknown_error,0,"Unknown msg kind %d",kind);
  }
   
  gras_datadesc_send(sock, string_type,   &msgtype->name);
  if (kind == e_gras_msg_kind_rpcerror) {
     /* error on remote host, carfull, payload is an exception */
    gras_datadesc_send(sock, gras_datadesc_by_name("ex_t"),payload);
  } else {
     /* regular message */
     if (msgtype->ctn_type)
       gras_datadesc_send(sock, msgtype->ctn_type, payload);
  }
   
  gras_trp_flush(sock);
}

/*
 * receive the next message on the given socket.  
 */
void
gras_msg_recv(gras_socket_t    sock,
	      gras_msg_t       msg) {

  xbt_ex_t e;
  static gras_datadesc_type_t string_type=NULL;
  static gras_datadesc_type_t ulong_type=NULL;
  char header[6];
  int cpt;
  int r_arch;
  char *msg_name=NULL;
  char c_kind;

  xbt_assert1(!gras_socket_is_meas(sock), 
  	      "Asked to receive a message on the measurement socket %p", sock);
  if (!string_type) {
    string_type=gras_datadesc_by_name("string");
    xbt_assert(string_type);
  }
  if (!ulong_type) {
    ulong_type = gras_datadesc_by_name("unsigned long int");
    xbt_assert(ulong_type);
  }

  
  TRY {
    gras_trp_recv(sock, header, 6);
    gras_trp_recv(sock, &c_kind, 1);
    msg->kind=(e_gras_msg_kind_t)c_kind;
  } CATCH(e) {
    RETHROW0("Exception caught while trying to get the mesage header: %s");
  }

  for (cpt=0; cpt<4; cpt++)
    if (header[cpt] != _GRAS_header[cpt])
      THROW2(mismatch_error,0,
	     "Incoming bytes do not look like a GRAS message (header='%.4s' not '%.4s')",
	     header,_GRAS_header);
  if (header[4] != _GRAS_header[4]) 
    THROW2(mismatch_error,0,"GRAS protocol mismatch (got %d, use %d)",
	   (int)header[4], (int)_GRAS_header[4]);
  r_arch = (int)header[5];
  DEBUG2("Handle an incoming message using protocol %d (remote is %s)",
	 (int)header[4],gras_datadesc_arch_name(r_arch));

  switch (msg->kind) {
  case e_gras_msg_kind_oneway: 
    break;
    
  case e_gras_msg_kind_rpccall:
  case e_gras_msg_kind_rpcanswer:
  case e_gras_msg_kind_rpcerror:
    gras_datadesc_recv(sock,ulong_type,r_arch, &msg->ID);
    break;
    
  default:
    THROW_IMPOSSIBLE;
  }

  gras_datadesc_recv(sock, string_type, r_arch, &msg_name);
  TRY {
    msg->type = (gras_msgtype_t)xbt_set_get_by_name(_gras_msgtype_set,msg_name);
  } CATCH(e) {
    /* FIXME: Survive unknown messages */
    RETHROW1("Exception caught while retrieving the type associated to messages '%s' : %s",
	     msg_name);
  }
  free(msg_name);

  if (msg->kind == e_gras_msg_kind_rpcerror) {
    /* error on remote host. Carfull with that exception, eugene */
    msg->payl_size=gras_datadesc_size(gras_datadesc_by_name("ex_t"));
    msg->payl=xbt_malloc(msg->payl_size);
    gras_datadesc_recv(sock, gras_datadesc_by_name("ex_t"), r_arch, msg->payl);

  } else {
     /* regular message */
     if (msg->type->ctn_type) {
	msg->payl_size=gras_datadesc_size(msg->type->ctn_type);
	xbt_assert2(msg->payl_size > 0,
		    "%s %s",
		    "Dynamic array as payload is forbided for now (FIXME?).",
		    "Reference to dynamic array is allowed.");
	msg->payl = xbt_malloc(msg->payl_size);
	gras_datadesc_recv(sock, msg->type->ctn_type, r_arch, msg->payl);
     } else {
	msg->payl = NULL;
	msg->payl_size = 0;
     }
  }
}
