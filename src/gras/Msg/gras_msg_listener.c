/* Thread in charge of listening the network and queuing incoming messages  */

/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

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
  xbt_queue_t incomming_messages;       /* messages received from the wire and still to be used by master */
  xbt_queue_t socks_to_close;   /* let the listener close the sockets, since it may be selecting on them. Darwin don't like this trick */
  gras_socket_t wakeup_sock_listener_side;
  gras_socket_t wakeup_sock_master_side;
  xbt_mutex_t init_mutex;       /* both this mutex and condition are used at initialization to make sure that */
  xbt_cond_t init_cond;         /* the main thread speaks to the listener only once it is started (FIXME: It would be easier using a semaphore, if only semaphores were in xbt_synchro) */
  xbt_thread_t listener;
} s_gras_msg_listener_t;

static void listener_function(void *p)
{
  gras_msg_listener_t me = (gras_msg_listener_t) p;
  gras_msg_t msg;
  xbt_ex_t e;
  gras_msgtype_t msg_wakeup_listener_t =
      gras_msgtype_by_name("_wakeup_listener");
  DEBUG0("I'm the listener");

  /* get a free socket for the receiving part of the listener */
  me->wakeup_sock_listener_side =
      gras_socket_server_range(5000, 6000, -1, 0);

  /* wake up the launcher */
  xbt_mutex_acquire(me->init_mutex);
  xbt_cond_signal(me->init_cond);
  xbt_mutex_release(me->init_mutex);

  /* Main loop */
  while (1) {
    msg = gras_msg_recv_any();
    if (msg->type != msg_wakeup_listener_t) {
      VERB1("Got a '%s' message. Queue it for handling by main thread",
            gras_msgtype_get_name(msg->type));
      xbt_queue_push(me->incomming_messages, msg);
    } else {
      char got = *(char *) msg->payl;
      if (got == '1') {
        VERB0("Asked to get awake");
        free(msg->payl);
        free(msg);
      } else {
        VERB0("Asked to die");
        //gras_socket_close(me->wakeup_sock_listener_side);
        free(msg->payl);
        free(msg);
        return;
      }
    }
    /* empty the list of sockets to trash */
    TRY {
      while (1) {
        int sock;
        xbt_queue_shift_timed(me->socks_to_close, &sock, 0);
        if (tcp_close(sock) < 0) {
#ifdef _XBT_WIN32
          WARN2("error while closing tcp socket %d: %d\n", sock,
                sock_errno);
#else
          WARN3("error while closing tcp socket %d: %d (%s)\n",
                sock, sock_errno, sock_errstr(sock_errno));
#endif
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

gras_msg_listener_t gras_msg_listener_launch(xbt_queue_t msg_received)
{
  gras_msg_listener_t arg = xbt_new0(s_gras_msg_listener_t, 1);

  VERB0("Launch listener");
  arg->incomming_messages = msg_received;
  arg->socks_to_close = xbt_queue_new(0, sizeof(int));
  arg->init_mutex = xbt_mutex_init();
  arg->init_cond = xbt_cond_init();

  /* declare the message used to awake the listener from the master */
  gras_msgtype_declare("_wakeup_listener", gras_datadesc_by_name("char"));

  /* actually start the thread, and */
  /* wait for the listener to initialize before we connect to its socket */
  xbt_mutex_acquire(arg->init_mutex);
  arg->listener =
      xbt_thread_create("listener", listener_function, arg,
                        1 /*joinable */ );
  xbt_cond_wait(arg->init_cond, arg->init_mutex);
  xbt_mutex_release(arg->init_mutex);
  xbt_cond_destroy(arg->init_cond);
  xbt_mutex_destroy(arg->init_mutex);

  /* Connect the other part of the socket */
  arg->wakeup_sock_master_side =
      gras_socket_client(gras_os_myname(),
                         gras_socket_my_port
                         (arg->wakeup_sock_listener_side));
  return arg;
}

#include "gras/Virtu/virtu_private.h"   /* procdata_t content */
void gras_msg_listener_shutdown()
{
  gras_procdata_t *pd = gras_procdata_get();
  char kill = '0';
  DEBUG0("Listener quit");

  if (pd->listener)
    gras_msg_send(pd->listener->wakeup_sock_master_side,
                  "_wakeup_listener", &kill);

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
    gras_msg_send(pd->listener->wakeup_sock_master_side,
                  "_wakeup_listener", &c);
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
