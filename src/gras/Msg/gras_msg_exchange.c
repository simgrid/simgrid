/* gras message exchanges                                                   */

/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "xbt/ex_interface.h"
#include "gras/Msg/msg_private.h"
#include "gras/Virtu/virtu_interface.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(gras_msg);


char _GRAS_header[6];
const char *e_gras_msg_kind_names[e_gras_msg_kind_count] =
    { "UNKNOWN", "ONEWAY", "RPC call", "RPC answer", "RPC error" };


/** \brief Waits for a message to come in over a given socket.
 *
 * @param timeout: How long should we wait for this message.
 * @param msgt_want: type of awaited msg (or NULL if I'm enclined to accept any message)
 * @param expe_want: awaited expeditot (match on hostname, not port; NULL if not relevant)
 * @param filter: function returning true or false when passed a payload. Messages for which it returns false are not selected. (NULL if not relevant)
 * @param filter_ctx: context passed as second argument of the filter (a pattern to match?)
 * @param[out] msg_got: where to write the message we got
 *
 * Every message of another type received before the one waited will be queued
 * and used by subsequent call to this function or gras_msg_handle().
 */

void
gras_msg_wait_ext_(double timeout,
                   gras_msgtype_t msgt_want,
                   gras_socket_t expe_want,
                   gras_msg_filter_t filter,
                   void *filter_ctx, gras_msg_t msg_got)
{

  s_gras_msg_t msg;
  double start, now;
  gras_msg_procdata_t pd =
      (gras_msg_procdata_t) gras_libdata_by_id(gras_msg_libdata_id);
  unsigned int cpt;

  xbt_assert0(msg_got, "msg_got is an output parameter");

  start = gras_os_time();
  VERB2("Waiting for message '%s' for %fs",
        msgt_want ? msgt_want->name : "(any)", timeout);

  xbt_dynar_foreach(pd->msg_waitqueue, cpt, msg) {
    if ((!msgt_want || (msg.type->code == msgt_want->code))
        && (!expe_want || (!strcmp(gras_socket_peer_name(msg.expe),
                                   gras_socket_peer_name(expe_want))))
        && (!filter || filter(&msg, filter_ctx))) {

      memcpy(msg_got, &msg, sizeof(s_gras_msg_t));
      xbt_dynar_cursor_rm(pd->msg_waitqueue, &cpt);
      VERB0("The waited message was queued");
      return;
    }
  }

  xbt_dynar_foreach(pd->msg_queue, cpt, msg) {
    if ((!msgt_want || (msg.type->code == msgt_want->code))
        && (!expe_want || (!strcmp(gras_socket_peer_name(msg.expe),
                                   gras_socket_peer_name(expe_want))))
        && (!filter || filter(&msg, filter_ctx))) {

      memcpy(msg_got, &msg, sizeof(s_gras_msg_t));
      xbt_dynar_cursor_rm(pd->msg_queue, &cpt);
      VERB0("The waited message was queued");
      return;
    }
  }

  while (1) {
    int need_restart;
    xbt_ex_t e;

  restart_receive:             /* Goto here when the receive of a message failed */
    need_restart = 0;
    now = gras_os_time();
    memset(&msg, 0, sizeof(msg));

    TRY {
      xbt_queue_shift_timed(pd->msg_received, &msg,
                            timeout ? timeout - now + start : 0);
    }
    CATCH(e) {
      if (e.category == system_error &&
          !strncmp("Socket closed by remote side", e.msg,
                   strlen("Socket closed by remote side"))) {
        xbt_ex_free(e);
        need_restart = 1;
      } else {
        RETHROW;
      }
    }
    if (need_restart)
      goto restart_receive;

    DEBUG0("Got a message from the socket");

    if ((!msgt_want || (msg.type->code == msgt_want->code))
        && (!expe_want || (!strcmp(gras_socket_peer_name(msg.expe),
                                   gras_socket_peer_name(expe_want))))
        && (!filter || filter(&msg, filter_ctx))) {

      memcpy(msg_got, &msg, sizeof(s_gras_msg_t));
      DEBUG0("Message matches expectations. Use it.");
      return;
    }
    DEBUG0("Message does not match expectations. Queue it.");

    /* not expected msg type. Queue it for later */
    xbt_dynar_push(pd->msg_queue, &msg);

    now = gras_os_time();
    if (now - start + 0.001 > timeout) {
      THROW1(timeout_error, now - start + 0.001 - timeout,
             "Timeout while waiting for msg '%s'",
             msgt_want ? msgt_want->name : "(any)");
    }
  }

  THROW_IMPOSSIBLE;
}

/** \brief Waits for a message to come in over a given socket.
 *
 * @param timeout: How long should we wait for this message.
 * @param msgt_want: type of awaited msg
 * @param[out] expeditor: where to create a socket to answer the incomming message
 * @param[out] payload: where to write the payload of the incomming message
 * @return the error code (or no_error).
 *
 * Every message of another type received before the one waited will be queued
 * and used by subsequent call to this function or gras_msg_handle().
 */
void
gras_msg_wait_(double timeout,
               gras_msgtype_t msgt_want,
               gras_socket_t * expeditor, void *payload)
{
  s_gras_msg_t msg;

  gras_msg_wait_ext_(timeout, msgt_want, NULL, NULL, NULL, &msg);

  if (msgt_want->ctn_type) {
    xbt_assert1(payload,
                "Message type '%s' convey a payload you must accept",
                msgt_want->name);
  } else {
    xbt_assert1(!payload,
                "No payload was declared for message type '%s'",
                msgt_want->name);
  }

  if (payload) {
    memcpy(payload, msg.payl, msg.payl_size);
    free(msg.payl);
  }

  if (expeditor)
    *expeditor = msg.expe;
}

static int gras_msg_wait_or_filter(gras_msg_t msg, void *ctx)
{
  xbt_dynar_t dyn = (xbt_dynar_t) ctx;
  int res = xbt_dynar_member(dyn, msg->type);
  if (res)
    VERB1("Got matching message (type=%s)", msg->type->name);
  else
    VERB0("Got message not matching our expectations");
  return res;
}

/** \brief Waits for a message to come in over a given socket.
 *
 * @param timeout: How long should we wait for this message.
 * @param msgt_want: a dynar containing all accepted message type
 * @param[out] ctx: the context of received message (in case it's a RPC call we want to answer to)
 * @param[out] msgt_got: indice in the dynar of the type of the received message
 * @param[out] payload: where to write the payload of the incomming message
 * @return the error code (or no_error).
 *
 * Every message of a type not in the accepted list received before the one
 * waited will be queued and used by subsequent call to this function or
 * gras_msg_handle().
 *
 * If you are interested in the context, pass the address of a s_gras_msg_cb_ctx_t variable.
 */
void gras_msg_wait_or(double timeout,
                      xbt_dynar_t msgt_want,
                      gras_msg_cb_ctx_t * ctx, int *msgt_got,
                      void *payload)
{
  s_gras_msg_t msg;

  VERB1("Wait %f seconds for several message types", timeout);
  gras_msg_wait_ext_(timeout,
                     NULL, NULL,
                     &gras_msg_wait_or_filter, (void *) msgt_want, &msg);

  if (msg.type->ctn_type) {
    xbt_assert1(payload,
                "Message type '%s' convey a payload you must accept",
                msg.type->name);
  }
  /* don't check the other side since some of the types may have a payload */
  if (payload && msg.type->ctn_type) {
    memcpy(payload, msg.payl, msg.payl_size);
    free(msg.payl);
  }

  if (ctx)
    *ctx = gras_msg_cb_ctx_new(msg.expe, msg.type, msg.ID,
                               (msg.kind == e_gras_msg_kind_rpccall), 60);

  if (msgt_got)
    *msgt_got = xbt_dynar_search(msgt_want, msg.type);
}


/** \brief Send the data pointed by \a payload as a message of type
 * \a msgtype to the peer \a sock */
void gras_msg_send_(gras_socket_t sock, gras_msgtype_t msgtype,
                    void *payload)
{

  if (msgtype->ctn_type) {
    xbt_assert1(payload,
                "Message type '%s' convey a payload you must provide",
                msgtype->name);
  } else {
    xbt_assert1(!payload,
                "No payload was declared for message type '%s'",
                msgtype->name);
  }

  DEBUG2("Send a oneway message of type '%s'. Payload=%p",
         msgtype->name, payload);
  gras_msg_send_ext(sock, e_gras_msg_kind_oneway, 0, msgtype, payload);
  VERB2("Sent a oneway message of type '%s'. Payload=%p",
        msgtype->name, payload);
}

/** @brief Handle all messages arriving within the given period
 *
 * @param period: How long to wait for incoming messages (in seconds)
 *
 * Messages are dealed with just like gras_msg_handle() would do. The
 * difference is that gras_msg_handle() handles at most one message (or wait up
 * to timeout second when no message arrives) while this function handles any
 * amount of messages, and lasts the given period in any case.
 */
void gras_msg_handleall(double period)
{
  xbt_ex_t e;
  double begin = gras_os_time();
  double now;

  do {
    now = gras_os_time();
    TRY {
      if (period - now + begin > 0)
        gras_msg_handle(period - now + begin);
    }
    CATCH(e) {
      if (e.category != timeout_error)
        RETHROW0("Error while waiting for messages: %s");
      xbt_ex_free(e);
    }
    /* Epsilon to avoid numerical stability issues were the waited interval is so small that the global clock cannot notice the increment */
  } while (period - now + begin > 0);
}

/** @brief Handle an incomming message or timer (or wait up to \a timeOut seconds)
 *
 * @param timeOut: How long to wait for incoming messages (in seconds)
 * @return the error code (or no_error).
 *
 * Any message arriving in the given interval is passed to the callbacks.
 *
 * @sa gras_msg_handleall().
 */
void gras_msg_handle(double timeOut)
{

  double untiltimer;

  unsigned int cpt;
  int volatile ran_ok;

  s_gras_msg_t msg;

  gras_msg_procdata_t pd =
      (gras_msg_procdata_t) gras_libdata_by_id(gras_msg_libdata_id);
  gras_cblist_t *list = NULL;
  gras_msg_cb_t cb;
  s_gras_msg_cb_ctx_t ctx;

  int timerexpected, timeouted;
  xbt_ex_t e;

  VERB1("Handling message within the next %.2fs", timeOut);

  untiltimer = gras_msg_timer_handle();
  DEBUG1("Next timer in %f sec", untiltimer);
  if (untiltimer == 0.0) {
    /* A timer was already elapsed and handled */
    return;
  }
  if (untiltimer != -1.0) {
    timerexpected = 1;
    timeOut = MIN(timeOut, untiltimer);
  } else {
    timerexpected = 0;
  }

  /* get a message (from the queue or from the net) */
  timeouted = 0;
  if (xbt_dynar_length(pd->msg_queue)) {
    DEBUG0("Get a message from the queue");
    xbt_dynar_shift(pd->msg_queue, &msg);
  } else {
    TRY {
      xbt_queue_shift_timed(pd->msg_received, &msg, timeOut);
      //      msg.expe = gras_trp_select(timeOut);
    }
    CATCH(e) {
      if (e.category != timeout_error)
        RETHROW;
      DEBUG0("Damn. Timeout while getting a message from the queue");
      xbt_ex_free(e);
      timeouted = 1;
    }
  }

  if (timeouted) {
    if (timerexpected) {

      /* A timer elapsed before the arrival of any message even if we select()ed a bit */
      untiltimer = gras_msg_timer_handle();
      if (untiltimer == 0.0) {
        /* we served a timer, we're done */
        return;
      } else {
        xbt_assert1(untiltimer > 0, "Negative timer (%f). I'm 'puzzeled'",
                    untiltimer);
        WARN1
            ("No timer elapsed, in contrary to expectations (next in %f sec)",
             untiltimer);
        THROW1(timeout_error, 0,
               "No timer elapsed, in contrary to expectations (next in %f sec)",
               untiltimer);
      }

    } else {
      /* select timeouted, and no timer elapsed. Nothing to do */
      THROW1(timeout_error, 0, "No new message or timer (delay was %f)",
             timeOut);
    }

  }

  /* A message was already there or arrived in the meanwhile. handle it */
  xbt_dynar_foreach(pd->cbl_list, cpt, list) {
    if (list->id == msg.type->code) {
      break;
    } else {
      list = NULL;
    }
  }
  if (!list) {
    INFO4
        ("No callback for message '%s' (type:%s) from %s:%d. Queue it for later gras_msg_wait() use.",
         msg.type->name, e_gras_msg_kind_names[msg.kind],
         gras_socket_peer_name(msg.expe), gras_socket_peer_port(msg.expe));
    xbt_dynar_push(pd->msg_waitqueue, &msg);
    return;                     /* FIXME: maybe we should call ourselves again until the end of the timer or a proper msg is got */
  }

  ctx.expeditor = msg.expe;
  ctx.ID = msg.ID;
  ctx.msgtype = msg.type;
  ctx.answer_due = (msg.kind == e_gras_msg_kind_rpccall);

  switch (msg.kind) {
  case e_gras_msg_kind_oneway:
  case e_gras_msg_kind_rpccall:
    ran_ok = 0;
    TRY {
      xbt_dynar_foreach(list->cbs, cpt, cb) {
        if (!ran_ok) {
          DEBUG4
              ("Use the callback #%d (@%p) for incomming msg '%s' (payload_size=%d)",
               cpt + 1, cb, msg.type->name, msg.payl_size);
          if (!(*cb) (&ctx, msg.payl)) {
            /* cb handled the message */
            free(msg.payl);
            ran_ok = 1;
          }
        }
      }
    }
    CATCH(e) {
      free(msg.payl);
      if (msg.type->kind == e_gras_msg_kind_rpccall) {
        char *old_file = e.file;
        /* The callback raised an exception, propagate it on the network */
        if (!e.remote) {
          /* Make sure we reduce the file name to its basename to avoid issues in tests */
          char *new_file = strrchr(e.file, '/');
          if (new_file)
            e.file = new_file;
          /* the exception is born on this machine */
          e.host = (char *) gras_os_myname();
          xbt_ex_setup_backtrace(&e);
        }
        INFO5
            ("Propagate %s exception ('%s') from '%s' RPC cb back to %s:%d",
             (e.remote ? "remote" : "local"), e.msg, msg.type->name,
             gras_socket_peer_name(msg.expe),
             gras_socket_peer_port(msg.expe));
        if (XBT_LOG_ISENABLED(gras_msg, xbt_log_priority_verbose))
          xbt_ex_display(&e);
        gras_msg_send_ext(msg.expe, e_gras_msg_kind_rpcerror,
                          msg.ID, msg.type, &e);
        e.file = old_file;
        xbt_ex_free(e);
        ctx.answer_due = 0;
        ran_ok = 1;
      } else {
        RETHROW4
            ("Callback #%d (@%p) to message '%s' (payload size: %d) raised an exception: %s",
             cpt + 1, cb, msg.type->name, msg.payl_size);
      }
    }

    xbt_assert1(!ctx.answer_due,
                "Bug in user code: RPC callback to message '%s' didn't call gras_msg_rpcreturn",
                msg.type->name);
    if (ctx.answer_due)
      CRITICAL1
          ("BUGS BOTH IN USER CODE (RPC callback to message '%s' didn't call gras_msg_rpcreturn) "
           "AND IN SIMGRID (process wasn't killed by an assert)",
           msg.type->name);
    if (!ran_ok)
      THROW1(mismatch_error, 0,
             "Message '%s' refused by all registered callbacks (maybe your callback misses a 'return 0' at the end)",
             msg.type->name);
    /* FIXME: gras_datadesc_free not implemented => leaking the payload */
    break;


  case e_gras_msg_kind_rpcanswer:
    INFO3("Unexpected RPC answer discarded (type: %s; from:%s:%d)",
          msg.type->name, gras_socket_peer_name(msg.expe),
          gras_socket_peer_port(msg.expe));
    WARN0
        ("FIXME: gras_datadesc_free not implemented => leaking the payload");
    return;

  case e_gras_msg_kind_rpcerror:
    INFO3("Unexpected RPC error discarded (type: %s; from:%s:%d)",
          msg.type->name, gras_socket_peer_name(msg.expe),
          gras_socket_peer_port(msg.expe));
    WARN0
        ("FIXME: gras_datadesc_free not implemented => leaking the payload");
    return;

  default:
    THROW1(unknown_error, 0,
           "Cannot handle messages of kind %d yet", msg.type->kind);
  }

}
