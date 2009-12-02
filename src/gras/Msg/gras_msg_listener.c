/* $Id$ */

/* Thread in charge of listening the network and queuing incoming messages  */

/* Copyright (c) 2007 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras/Msg/msg_private.h"
#include "gras/Transport/transport_private.h"
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras_msg_read, gras_msg,
                                "Message reader thread");

#include "xbt/ex.h"
#include "xbt/queue.h"
#include "xbt/synchro.h"

#include "gras/Transport/transport_interface.h" /* gras_select */

typedef struct s_gras_msg_listener_ {
  xbt_queue_t incomming_messages;
  xbt_queue_t socks_to_close;   /* let the listener close the sockets, since it may be selecting on them. Darwin don't like this trick */
  gras_socket_t wakeup_sock_listener_side;
  gras_socket_t wakeup_sock_master_side;
  xbt_thread_t listener;
} s_gras_msg_listener_t;

static void listener_function(void *p)
{
  gras_msg_listener_t me = (gras_msg_listener_t) p;
  s_gras_msg_t msg;
  xbt_ex_t e;
  gras_msgtype_t msg_wakeup_listener_t =
    gras_msgtype_by_name("_wakeup_listener");
  DEBUG0("I'm the listener");
  while (1) {
    DEBUG0("Selecting");
    msg.expe = gras_trp_select(-1);
    DEBUG0("Select returned something");
    gras_msg_recv(msg.expe, &msg);
    if (msg.type != msg_wakeup_listener_t)
      xbt_queue_push(me->incomming_messages, &msg);
    else {
      char got = *(char *) msg.payl;
      if (got == '1') {
        VERB0("Asked to get awake");
        free(msg.payl);
      } else {
        VERB0("Asked to die");
        //        gras_socket_close(me->wakeup_sock_listener_side);
        free(msg.payl);
        return;
      }
    }
    /* empty the list of sockets to trash */
    TRY {
      while (1) {
        int sock;
        xbt_queue_shift_timed(me->socks_to_close, &sock, 0);
        if (tcp_close(sock) < 0) {
          WARN3("error while closing tcp socket %d: %d (%s)\n",
                sock, sock_errno, sock_errstr(sock_errno));
        }
      }
    }
    CATCH(e) {
      if (e.category != timeout_error)
        RETHROW;
      xbt_ex_free(e);
    }
  }
}

gras_msg_listener_t gras_msg_listener_launch(xbt_queue_t msg_exchange)
{
  gras_msg_listener_t arg = xbt_new0(s_gras_msg_listener_t, 1);
  int my_port;

  DEBUG0("Launch listener");
  arg->incomming_messages = msg_exchange;
  arg->socks_to_close = xbt_queue_new(0, sizeof(int));

  /* get a free socket for the receiving part of the listener, taking care that it does not get saved as "myport" number */
  my_port =
    ((gras_trp_procdata_t) gras_libdata_by_id(gras_trp_libdata_id))->myport;
  arg->wakeup_sock_listener_side =
    gras_socket_server_range(5000, 6000, -1, 0);
  ((gras_trp_procdata_t) gras_libdata_by_id(gras_trp_libdata_id))->myport =
    my_port;
  /* Connect the other part of the socket */
  arg->wakeup_sock_master_side =
    gras_socket_client(gras_os_myname(),
                       gras_socket_my_port(arg->wakeup_sock_listener_side));

  /* declare the message used to awake the listener from the master */
  gras_msgtype_declare("_wakeup_listener", gras_datadesc_by_name("char"));

  /* actually start the thread */
  arg->listener = xbt_thread_create("listener", listener_function, arg,1/*joinable*/);
  gras_os_sleep(0);             /* TODO: useless? give the listener a chance to initialize even if the main is empty and we cancel it right afterward */
  return arg;
}

#include "gras/Virtu/virtu_private.h"   /* procdata_t content */
void gras_msg_listener_shutdown()
{
  gras_procdata_t *pd = gras_procdata_get();
  char kill = '0';
  DEBUG0("Listener quit");

  if (pd->listener)
    gras_msg_send(pd->listener->wakeup_sock_master_side, "_wakeup_listener",
          &kill);

  xbt_thread_join(pd->listener->listener);

  //  gras_socket_close(pd->listener->wakeup_sock_master_side); FIXME: uncommenting this leads to deadlock at terminaison
  xbt_queue_free(&pd->listener->incomming_messages);
  xbt_queue_free(&pd->listener->socks_to_close);
  xbt_free(pd->listener);
}

void gras_msg_listener_awake()
{
  gras_procdata_t *pd;
  char c = '1';

  DEBUG0("Awaking the listener");
  pd = gras_procdata_get();
  if (pd->listener) {
    gras_msg_send(pd->listener->wakeup_sock_master_side, "_wakeup_listener",
                  &c);
  }
}

void gras_msg_listener_close_socket(int sd)
{
  gras_procdata_t *pd = gras_procdata_get();
  if (pd->listener) {
    xbt_queue_push(pd->listener->socks_to_close, &sd);
    gras_msg_listener_awake();
  } else {
    /* do it myself */
    tcp_close(sd);
  }
}
