/* messaging - Function related to messaging code specific to RL            */

/* Copyright (c) 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "xbt/datadesc.h"
#include "xbt/datadesc/datadesc_interface.h"
#include "xbt/socket.h"
#include "gras/Msg/msg_private.h"
#include "gras/Transport/transport_interface.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(gras_msg);

void gras_msg_recv(xbt_socket_t sock, gras_msg_t msg);

gras_msg_t gras_msg_recv_any(void)
{
  gras_msg_t msg = xbt_new0(s_gras_msg_t, 1);
  msg->expe = gras_trp_select(-1);
  XBT_DEBUG("Select returned something");
  gras_msg_recv(msg->expe, msg);
  return msg;
}

void gras_msg_send_ext(xbt_socket_t sock,
                       e_gras_msg_kind_t kind,
                       unsigned long int ID,
                       gras_msgtype_t msgtype, void *payload)
{

  static xbt_datadesc_type_t string_type = NULL;
  static xbt_datadesc_type_t ulong_type = NULL;
  char c_kind = (char) kind;

  xbt_assert(msgtype, "Cannot send the NULL message");

  if (!string_type) {
    string_type = xbt_datadesc_by_name("string");
    xbt_assert(string_type);
  }
  if (!ulong_type) {
    ulong_type = xbt_datadesc_by_name("unsigned long int");
    xbt_assert(ulong_type);
  }

  XBT_DEBUG("send '%s' to %s:%d", msgtype->name,
         xbt_socket_peer_name(sock), xbt_socket_peer_port(sock));
  xbt_trp_send(sock, _GRAS_header, 6, 1 /* stable */ );
  xbt_trp_send(sock, &c_kind, 1, 1 /* stable */ );
  switch (kind) {
  case e_gras_msg_kind_oneway:
    break;

  case e_gras_msg_kind_rpccall:
  case e_gras_msg_kind_rpcanswer:
  case e_gras_msg_kind_rpcerror:
    xbt_datadesc_send(sock, ulong_type, &ID);
    break;

  default:
    THROWF(unknown_error, 0, "Unknown msg kind %d", (int)kind);
  }

  xbt_datadesc_send(sock, string_type, &msgtype->name);
  if (kind == e_gras_msg_kind_rpcerror) {
    /* error on remote host, carfull, payload is an exception */
    xbt_datadesc_send(sock, xbt_datadesc_by_name("ex_t"), payload);
  } else if (kind == e_gras_msg_kind_rpcanswer) {
    if (msgtype->answer_type)
      xbt_datadesc_send(sock, msgtype->answer_type, payload);
  } else {
    /* regular message */
    if (msgtype->ctn_type)
      xbt_datadesc_send(sock, msgtype->ctn_type, payload);
  }

  xbt_trp_flush(sock);
}

const char *hexa_str(unsigned char *data, int size, int downside);


/*
 * receive the next message on the given socket.
 */
void gras_msg_recv(xbt_socket_t sock, gras_msg_t msg)
{

  xbt_ex_t e;
  static xbt_datadesc_type_t string_type = NULL;
  static xbt_datadesc_type_t ulong_type = NULL;
  char header[6];
  int cpt;
  int r_arch;
  char *msg_name = NULL;
  char c_kind;

  xbt_assert(!xbt_socket_is_meas(sock),
              "Asked to receive a message on the measurement socket %p",
              sock);
  if (!string_type) {
    string_type = xbt_datadesc_by_name("string");
    xbt_assert(string_type);
  }
  if (!ulong_type) {
    ulong_type = xbt_datadesc_by_name("unsigned long int");
    xbt_assert(ulong_type);
  }


  TRY {
    xbt_trp_recv(sock, header, 6);
    xbt_trp_recv(sock, &c_kind, 1);
    msg->kind = (e_gras_msg_kind_t) c_kind;
  }
  CATCH_ANONYMOUS {
    RETHROWF
        ("Exception caught while trying to get the message header: %s");
  }

  for (cpt = 0; cpt < 4; cpt++)
    if (header[cpt] != _GRAS_header[cpt])
      THROWF(mismatch_error, 0,
             "Incoming bytes do not look like a GRAS message (header='%s'  not '%.4s')",
             hexa_str((unsigned char *) header, 4, 0), _GRAS_header);
  if (header[4] != _GRAS_header[4])
    THROWF(mismatch_error, 0, "GRAS protocol mismatch (got %d, use %d)",
           (int) header[4], (int) _GRAS_header[4]);
  r_arch = (int) header[5];

  switch (msg->kind) {
  case e_gras_msg_kind_oneway:
    break;

  case e_gras_msg_kind_rpccall:
  case e_gras_msg_kind_rpcanswer:
  case e_gras_msg_kind_rpcerror:
    xbt_datadesc_recv(sock, ulong_type, r_arch, &msg->ID);
    break;

  default:
    THROW_IMPOSSIBLE;
  }

  xbt_datadesc_recv(sock, string_type, r_arch, &msg_name);
  XBT_DEBUG
      ("Handle an incoming message '%s' (%s) using protocol %d (remote is %s)",
       msg_name, e_gras_msg_kind_names[msg->kind], (int) header[4],
       xbt_datadesc_arch_name(r_arch));

  TRY {
    msg->type =
        (gras_msgtype_t) xbt_set_get_by_name(_gras_msgtype_set, msg_name);
  }
  CATCH(e) {
    /* FIXME: Survive unknown messages */
    if (e.category == not_found_error) {
      xbt_ex_free(e);
      THROWF(not_found_error, 0,
             "Received an unknown message: %s (FIXME: should survive to these)",
             msg_name);
    } else
      RETHROWF
          ("Exception caught while retrieving the type associated to messages '%s' : %s",
           msg_name);
  }
  free(msg_name);

  if (msg->kind == e_gras_msg_kind_rpcerror) {
    /* error on remote host. Carfull with that exception, eugene */
    msg->payl_size = xbt_datadesc_size(xbt_datadesc_by_name("ex_t"));
    msg->payl = xbt_malloc(msg->payl_size);
    xbt_datadesc_recv(sock, xbt_datadesc_by_name("ex_t"), r_arch,
                       msg->payl);

  } else if (msg->kind == e_gras_msg_kind_rpcanswer) {
    /* answer to RPC */
    if (msg->type->answer_type) {
      msg->payl_size = xbt_datadesc_size(msg->type->answer_type);
      xbt_assert(msg->payl_size > 0,
                  "%s %s",
                  "Dynamic array as payload is forbided for now (FIXME?).",
                  "Reference to dynamic array is allowed.");
      msg->payl = xbt_malloc(msg->payl_size);
      xbt_datadesc_recv(sock, msg->type->answer_type, r_arch, msg->payl);
    } else {
      msg->payl = NULL;
      msg->payl_size = 0;
    }
  } else {
    /* regular message */
    if (msg->type->ctn_type) {
      msg->payl_size = xbt_datadesc_size(msg->type->ctn_type);
      xbt_assert(msg->payl_size > 0,
                  "%s %s",
                  "Dynamic array as payload is forbided for now (FIXME?).",
                  "Reference to dynamic array is allowed.");
      msg->payl = xbt_malloc(msg->payl_size);
      xbt_datadesc_recv(sock, msg->type->ctn_type, r_arch, msg->payl);
    } else {
      msg->payl = NULL;
      msg->payl_size = 0;
    }
  }
}
