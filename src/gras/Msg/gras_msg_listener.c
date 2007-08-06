/* $Id$ */

/* Thread in charge of listening the network and queuing incoming messages  */

/* Copyright (c) 2007 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras/Msg/msg_private.h"
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras_msg_read,gras_msg,"Message reader thread");

#include "xbt/ex.h"
#include "xbt/queue.h"
#include "xbt/synchro.h"

#include "gras/Transport/transport_interface.h" /* gras_select */

typedef struct s_gras_msg_listener_ {
  xbt_queue_t incomming_messages;
  xbt_thread_t listener;
} s_gras_msg_listener_t;

static void listener_function(void *p) {
  gras_msg_listener_t me = (gras_msg_listener_t)p;
  s_gras_msg_t msg;
  xbt_ex_t e;
  int found =0;
  DEBUG0("I'm the listener");
  while (1) {
    TRY {
      DEBUG0("Select for .5 sec");
      msg.expe = gras_trp_select(0.5);
      found =1;
      DEBUG0("Select returned something");
    }
    CATCH(e) {
      xbt_ex_free(e);			
    }
    if (found) {
      gras_msg_recv(msg.expe, &msg);
      xbt_queue_push(me->incomming_messages, &msg);
      found =0;
    }
  }
}

gras_msg_listener_t
gras_msg_listener_launch(xbt_queue_t msg_exchange){
  gras_msg_listener_t arg = xbt_new0(s_gras_msg_listener_t,1);

  DEBUG0("Launch listener");
  arg->incomming_messages = msg_exchange;

  arg->listener =  xbt_thread_create("listener",listener_function,arg);
  return arg;
}

void gras_msg_listener_shutdown(gras_msg_listener_t l) {
  DEBUG0("Listener quit");
  xbt_thread_cancel(l->listener);
  xbt_queue_free(&l->incomming_messages);
  xbt_free(l);
}
