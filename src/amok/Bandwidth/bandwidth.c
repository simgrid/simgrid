/* $Id$ */

/* amok_bandwidth - Bandwidth tests facilities                              */

/* Copyright (c) 2003-6 Martin Quinson.                                     */
/* Copyright (c) 2006   Ahmed Harbaoui.                                     */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "amok/Bandwidth/bandwidth_private.h"
#include "gras/messages.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(amok_bw, amok, "Bandwidth testing");


/******************************
 * Stuff global to the module *
 ******************************/

static short _amok_bw_initialized = 0;

/** @brief module initialization; all participating nodes must run this */
void amok_bw_init(void)
{

  if (!_amok_bw_initialized) {
    amok_bw_bw_init();
    amok_bw_sat_init();
  }

  amok_bw_bw_join();
  amok_bw_sat_join();

  _amok_bw_initialized++;
}

/** @brief module finalization */
void amok_bw_exit(void)
{
  if (!_amok_bw_initialized)
    return;

  amok_bw_bw_leave();
  amok_bw_sat_leave();

  _amok_bw_initialized--;
}

/* ***************************************************************************
 * Bandwidth tests
 * ***************************************************************************/
static int amok_bw_cb_bw_handshake(gras_msg_cb_ctx_t ctx, void *payload);
static int amok_bw_cb_bw_request(gras_msg_cb_ctx_t ctx, void *payload);

void amok_bw_bw_init()
{
  gras_datadesc_type_t bw_request_desc, bw_res_desc;

  /* Build the Bandwidth datatype descriptions */
  bw_request_desc = gras_datadesc_struct("s_bw_request_t");
  gras_datadesc_struct_append(bw_request_desc, "peer",
                              gras_datadesc_by_name("s_xbt_peer_t"));
  gras_datadesc_struct_append(bw_request_desc, "buf_size",
                              gras_datadesc_by_name("unsigned long int"));
  gras_datadesc_struct_append(bw_request_desc, "msg_size",
                              gras_datadesc_by_name("unsigned long int"));
  gras_datadesc_struct_append(bw_request_desc, "msg_amount",
                              gras_datadesc_by_name("unsigned long int"));
  gras_datadesc_struct_append(bw_request_desc, "min_duration",
                              gras_datadesc_by_name("double"));
  gras_datadesc_struct_close(bw_request_desc);
  bw_request_desc = gras_datadesc_ref("bw_request_t", bw_request_desc);

  bw_res_desc = gras_datadesc_struct("s_bw_res_t");
  gras_datadesc_struct_append(bw_res_desc, "timestamp",
                              gras_datadesc_by_name("unsigned int"));
  gras_datadesc_struct_append(bw_res_desc, "seconds",
                              gras_datadesc_by_name("double"));
  gras_datadesc_struct_append(bw_res_desc, "bw",
                              gras_datadesc_by_name("double"));
  gras_datadesc_struct_close(bw_res_desc);
  bw_res_desc = gras_datadesc_ref("bw_res_t", bw_res_desc);

  gras_msgtype_declare_rpc("BW handshake", bw_request_desc, bw_request_desc);

  gras_msgtype_declare_rpc("BW reask", bw_request_desc, NULL);
  gras_msgtype_declare("BW stop", NULL);

  gras_msgtype_declare_rpc("BW request", bw_request_desc, bw_res_desc);
}

void amok_bw_bw_join()
{
  gras_cb_register("BW request", &amok_bw_cb_bw_request);
  gras_cb_register("BW handshake", &amok_bw_cb_bw_handshake);
}

void amok_bw_bw_leave()
{
  gras_cb_unregister("BW request", &amok_bw_cb_bw_request);
  gras_cb_unregister("BW handshake", &amok_bw_cb_bw_handshake);
}

/**
 * \brief bandwidth measurement between localhost and \e peer
 * 
 * \arg peer: A (regular) socket at which the the host with which we should conduct the experiment can be contacted
 * \arg buf_size: Size of the socket buffer. If 0, a sain default is used (32k, but may change)
 * \arg msg_size: Size of each message sent. 
 * \arg msg_amount: Amount of such messages to exchange 
 * \arg min_duration: The minimum wanted duration. When the test message is too little, you tend to measure the latency. This argument allows you to force the test to take at least, say one second.
 * \arg sec: where the result (in seconds) should be stored. If the experiment was done several times because the first one was too short, this is the timing of the last run only.
 * \arg bw: observed Bandwidth (in byte/s) 
 *
 * Conduct a bandwidth test from the local process to the given peer.
 * This call is blocking until the end of the experiment.
 *
 * If the asked experiment lasts less than \a min_duration, another one will be
 * launched (and others, if needed). msg_size will be multiplicated by
 * MIN(20, (\a min_duration / measured_duration) *1.1) (plus 10% to be sure to eventually
 * reach the \a min_duration). In that case, the reported bandwidth and
 * duration are the ones of the last run. \a msg_size cannot go over 64Mb
 * because we need to malloc a block of this size in RL to conduct the
 * experiment, and we still don't want to visit the swap. In such case, the 
 * number of messages is increased instead of their size.
 *
 * Results are reported in last args, and sizes are in byte.
 * 
 * @warning: in SimGrid version 3.1 and previous, the experiment size were specified
 *           as the total amount of data to send and the msg_size. This
 *           was changed for the fool wanting to send more than MAXINT
 *           bytes in a fat pipe.
 * 
 */
void amok_bw_test(gras_socket_t peer,
                  unsigned long int buf_size,
                  unsigned long int msg_size,
                  unsigned long int msg_amount,
                  double min_duration, /*OUT*/ double *sec, double *bw)
{

  /* Measurement sockets for the experiments */
  gras_socket_t measMasterIn = NULL, measIn, measOut = NULL;
  int port;
  bw_request_t request, request_ack;
  xbt_ex_t e;
  int first_pass;

  for (port = 5000; port < 10000 && measMasterIn == NULL; port++) {
    TRY {
      measMasterIn = gras_socket_server_ext(++port, buf_size, 1);
    }
    CATCH(e) {
      measMasterIn = NULL;
      if (port == 10000 - 1) {
        RETHROW0("Error caught while opening a measurement socket: %s");
      } else {
        xbt_ex_free(e);
      }
    }
  }

  request = xbt_new0(s_bw_request_t, 1);
  request->buf_size = buf_size;
  request->msg_size = msg_size;
  request->msg_amount = msg_amount;
  request->peer.name = NULL;
  request->peer.port = gras_socket_my_port(measMasterIn);
  DEBUG6
    ("Handshaking with %s:%d to connect it back on my %d (bufsize=%ld, msg_size=%ld, msg_amount=%ld)",
     gras_socket_peer_name(peer), gras_socket_peer_port(peer),
     request->peer.port, request->buf_size, request->msg_size,
     request->msg_amount);

  TRY {
    gras_msg_rpccall(peer, 15, "BW handshake", &request, &request_ack);
  }
  CATCH(e) {
    RETHROW0("Error encountered while sending the BW request: %s");
  }
  measIn = gras_socket_meas_accept(measMasterIn);

  TRY {
    measOut = gras_socket_client_ext(gras_socket_peer_name(peer),
                                     request_ack->peer.port,
                                     request->buf_size, 1);
  }
  CATCH(e) {
    RETHROW2
      ("Error encountered while opening the measurement socket to %s:%d for BW test: %s",
       gras_socket_peer_name(peer), request_ack->peer.port);
  }
  DEBUG2("Got ACK; conduct the experiment (msg_size = %ld, msg_amount=%ld)",
         request->msg_size, request->msg_amount);

  *sec = 0;
  first_pass = 1;
  do {
    if (first_pass == 0) {
      double meas_duration = *sec;
      double increase;
      if (*sec != 0.0) {
        increase = (min_duration / meas_duration) * 1.1;
      } else {
        increase = 4;
      }
      /* Do not increase the exp size too fast since our decision would be based on wrong measurements */
      if (increase > 20)
        increase = 20;

      request->msg_size = request->msg_size * increase;

      /* Do not do too large experiments messages or the sensors 
         will start to swap to store one of them.
         And then increase the number of messages to compensate (check for overflow there, too) */
      if (request->msg_size > 64 * 1024 * 1024) {
        unsigned long int new_amount =
          ((request->msg_size / ((double) 64 * 1024 * 1024))
           * request->msg_amount) + 1;

        xbt_assert0(new_amount > request->msg_amount,
                    "Overflow on the number of messages! You must have a *really* fat pipe. Please fix your platform");
        request->msg_amount = new_amount;

        request->msg_size = 64 * 1024 * 1024;
      }

      VERB5
        ("The experiment was too short (%f sec<%f sec). Redo it with msg_size=%lu (nb_messages=%lu) (got %fMb/s)",
         meas_duration, min_duration, request->msg_size, request->msg_amount,
         ((double) request->msg_size) * ((double) request->msg_amount /
                                         (*sec) / 1024.0 / 1024.0));

      gras_msg_rpccall(peer, 60, "BW reask", &request, NULL);
    }

    first_pass = 0;
    *sec = gras_os_time();
    TRY {
      gras_socket_meas_send(measOut, 120, request->msg_size,
                            request->msg_amount);
      DEBUG0("Data sent. Wait ACK");
      gras_socket_meas_recv(measIn, 120, 1, 1);
    } CATCH(e) {
      gras_socket_close(measOut);
      gras_socket_close(measMasterIn);
      gras_socket_close(measIn);
      RETHROW0("Unable to conduct the experiment: %s");
    }
    *sec = gras_os_time() - *sec;
    if (*sec != 0.0) {
      *bw =
        ((double) request->msg_size) * ((double) request->msg_amount) /
        (*sec);
    }
    DEBUG1("Experiment done ; it took %f sec", *sec);
    if (*sec <= 0) {
      CRITICAL1("Nonpositive value (%f) found for BW test time.", *sec);
    }

  } while (*sec < min_duration);

  DEBUG2("This measurement was long enough (%f sec; found %f b/s). Stop peer",
         *sec, *bw);
  gras_msg_send(peer, "BW stop", NULL);

  free(request_ack);
  free(request);
  if (measIn != measMasterIn)
    gras_socket_close(measIn);
  gras_socket_close(measMasterIn);
  gras_socket_close(measOut);
}


/* Callback to the "BW handshake" message: 
   opens a server measurement socket,
   indicate its port in an "BW handshaked" message,
   receive the corresponding data on the measurement socket, 
   close the measurement socket

   sizes are in byte
*/
int amok_bw_cb_bw_handshake(gras_msg_cb_ctx_t ctx, void *payload)
{
  gras_socket_t expeditor = gras_msg_cb_ctx_from(ctx);
  gras_socket_t measMasterIn = NULL, measIn = NULL, measOut = NULL;
  bw_request_t request = *(bw_request_t *) payload;
  bw_request_t answer;
  xbt_ex_t e;
  int port;
  int tooshort = 1;
  gras_msg_cb_ctx_t ctx_reask;
  static xbt_dynar_t msgtwaited = NULL;

  DEBUG5
    ("Handshaked to connect to %s:%d (sizes: buf=%lu msg=%lu msg_amount=%lu)",
     gras_socket_peer_name(expeditor), request->peer.port, request->buf_size,
     request->msg_size, request->msg_amount);

  /* Build our answer */
  answer = xbt_new0(s_bw_request_t, 1);

  for (port = 6000; port <= 10000 && measMasterIn == NULL; port++) {
    TRY {
      measMasterIn = gras_socket_server_ext(port, request->buf_size, 1);
    }
    CATCH(e) {
      measMasterIn = NULL;
      if (port < 10000)
        xbt_ex_free(e);
      else
        /* FIXME: tell error to remote */
        RETHROW0
          ("Error encountered while opening a measurement server socket: %s");
    }
  }

  answer->buf_size = request->buf_size;
  answer->msg_size = request->msg_size;
  answer->msg_amount = request->msg_amount;
  answer->peer.port = gras_socket_my_port(measMasterIn);

  TRY {
    gras_msg_rpcreturn(60, ctx, &answer);
  }
  CATCH(e) {
    gras_socket_close(measMasterIn);
    /* FIXME: tell error to remote */
    RETHROW0("Error encountered while sending the answer: %s");
  }


  /* Don't connect asap to leave time to other side to enter the accept() */
  TRY {
    measOut = gras_socket_client_ext(gras_socket_peer_name(expeditor),
                                     request->peer.port,
                                     request->buf_size, 1);
  }
  CATCH(e) {
    RETHROW2
      ("Error encountered while opening a measurement socket back to %s:%d : %s",
       gras_socket_peer_name(expeditor), request->peer.port);
    /* FIXME: tell error to remote */
  }

  TRY {
    measIn = gras_socket_meas_accept(measMasterIn);
    DEBUG4
      ("BW handshake answered. buf_size=%lu msg_size=%lu msg_amount=%lu port=%d",
       answer->buf_size, answer->msg_size, answer->msg_amount,
       answer->peer.port);
  }
  CATCH(e) {
    gras_socket_close(measMasterIn);
    gras_socket_close(measIn);
    gras_socket_close(measOut);
    /* FIXME: tell error to remote ? */
    RETHROW0("Error encountered while opening the meas socket: %s");
  }

  if (!msgtwaited) {
    msgtwaited = xbt_dynar_new(sizeof(gras_msgtype_t), NULL);
    xbt_dynar_push(msgtwaited, gras_msgtype_by_name("BW stop"));
    xbt_dynar_push(msgtwaited, gras_msgtype_by_name("BW reask"));
  }

  while (tooshort) {
    void *payload;
    int msggot;
    TRY {
      gras_socket_meas_recv(measIn, 120, request->msg_size,
                            request->msg_amount);
      gras_socket_meas_send(measOut, 120, 1, 1);
    } CATCH(e) {
      gras_socket_close(measMasterIn);
      gras_socket_close(measIn);
      gras_socket_close(measOut);
      /* FIXME: tell error to remote ? */
      RETHROW0("Error encountered while receiving the experiment: %s");
    }
    gras_msg_wait_or(60, msgtwaited, &ctx_reask, &msggot, &payload);
    switch (msggot) {
    case 0:                    /* BW stop */
      tooshort = 0;
      break;
    case 1:                    /* BW reask */
      tooshort = 1;
      free(request);
      request = (bw_request_t) payload;
      VERB0("Return the reasking RPC");
      gras_msg_rpcreturn(60, ctx_reask, NULL);
    }
    gras_msg_cb_ctx_free(ctx_reask);
  }

  if (measIn != measMasterIn)
    gras_socket_close(measMasterIn);
  gras_socket_close(measIn);
  gras_socket_close(measOut);
  free(answer);
  free(request);
  VERB0("BW experiment done.");
  return 0;
}

/**
 * \brief request a bandwidth measurement between two remote peers
 *
 * \arg from_name: Name of the first peer 
 * \arg from_port: port on which the first process is listening for messages
 * \arg to_name: Name of the second peer 
 * \arg to_port: port on which the second process is listening (for messages, do not 
 * give a measurement socket here. The needed measurement sockets will be created 
 * automatically and negociated between the peers)
 * \arg buf_size: Size of the socket buffer. If 0, a sain default is used (32k, but may change)
 * \arg msg_size: Size of each message sent. 
 * \arg msg_amount: Amount of such data to exchange
 * \arg sec: where the result (in seconds) should be stored.
 * \arg bw: observed Bandwidth (in byte/s)
 *
 * Conduct a bandwidth test from the process from_peer:from_port to to_peer:to_port.
 * This call is blocking until the end of the experiment.
 *
 * @warning: in SimGrid version 3.1 and previous, the experiment size were specified
 *           as the total amount of data to send and the msg_size. This
 *           was changed for the fool wanting to send more than MAXINT
 *           bytes in a fat pipe.
 * 
 * Results are reported in last args, and sizes are in bytes.
 */
void amok_bw_request(const char *from_name, unsigned int from_port,
                     const char *to_name, unsigned int to_port,
                     unsigned long int buf_size,
                     unsigned long int msg_size,
                     unsigned long int msg_amount,
                     double min_duration, /*OUT*/ double *sec, double *bw)
{

  gras_socket_t sock;
  /* The request */
  bw_request_t request;
  bw_res_t result;
  request = xbt_new0(s_bw_request_t, 1);
  request->buf_size = buf_size;
  request->msg_size = msg_size;
  request->msg_amount = msg_amount;
  request->min_duration = min_duration;


  request->peer.name = (char *) to_name;
  request->peer.port = to_port;


  sock = gras_socket_client(from_name, from_port);



  DEBUG4("Ask for a BW test between %s:%d and %s:%d", from_name, from_port,
         to_name, to_port);
  gras_msg_rpccall(sock, 20 * 60, "BW request", &request, &result);

  if (sec)
    *sec = result->sec;
  if (bw)
    *bw = result->bw;

  VERB6("BW test (%s:%d -> %s:%d) took %f sec (%f kb/s)",
        from_name, from_port, to_name, to_port,
        result->sec, ((double) result->bw) / 1024.0);

  gras_socket_close(sock);
  free(result);
  free(request);
}

int amok_bw_cb_bw_request(gras_msg_cb_ctx_t ctx, void *payload)
{

  /* specification of the test to run, and our answer */
  bw_request_t request = *(bw_request_t *) payload;
  bw_res_t result = xbt_new0(s_bw_res_t, 1);
  gras_socket_t peer, asker;

  asker = gras_msg_cb_ctx_from(ctx);
  VERB6("Asked by %s:%d to conduct a bw XP with %s:%d (request: %ld %ld)",
        gras_socket_peer_name(asker), gras_socket_peer_port(asker),
        request->peer.name, request->peer.port,
        request->msg_size, request->msg_amount);
  peer = gras_socket_client(request->peer.name, request->peer.port);
  amok_bw_test(peer,
               request->buf_size, request->msg_size, request->msg_amount,
               request->min_duration, &(result->sec), &(result->bw));

  gras_msg_rpcreturn(240, ctx, &result);

  gras_os_sleep(1);
  gras_socket_close(peer);      /* FIXME: it should be blocking in RL until everything is sent */
  free(request->peer.name);
  free(request);
  free(result);

  return 0;
}

/** \brief builds a matrix of results of bandwidth measurement
 * 
 * @warning: in SimGrid version 3.1 and previous, the experiment size were specified
 *           as the total amount of data to send and the msg_size. This
 *           was changed for the fool wanting to send more than MAXINT
 *           bytes in a fat pipe.
 */
double *amok_bw_matrix(xbt_dynar_t peers,
                       int buf_size_bw, int msg_size_bw, int msg_amount_bw,
                       double min_duration)
{
  double sec;
  /* construction of matrices for bandwith and latency */


  unsigned int i, j;
  int len = xbt_dynar_length(peers);

  double *matrix_res = xbt_new0(double, len * len);
  xbt_peer_t p1, p2;

  xbt_dynar_foreach(peers, i, p1) {
    xbt_dynar_foreach(peers, j, p2) {
      if (i != j) {
        /* Mesurements of Bandwidth */
        amok_bw_request(p1->name, p1->port, p2->name, p2->port,
                        buf_size_bw, msg_size_bw, msg_amount_bw, min_duration,
                        &sec, &matrix_res[i * len + j]);
      }
    }
  }
  return matrix_res;
}
