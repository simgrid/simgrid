/* $Id$ */

/* amok_saturate - Link saturating facilities (for ALNeM's BW testing)      */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "amok/Bandwidth/bandwidth_private.h"
#include "gras/Msg/msg_private.h"       /* FIXME: This mucks with contextes to answer RPC directly */
#include "xbt/ex.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(amok_bw_sat, amok_bw,
                                "Everything concerning the SATuration part of the amok_bw module");

static int amok_bw_cb_sat_start(gras_msg_cb_ctx_t ctx, void *payload);
static int amok_bw_cb_sat_begin(gras_msg_cb_ctx_t ctx, void *payload);


void amok_bw_sat_init(void)
{
  gras_datadesc_type_t bw_res_desc = gras_datadesc_by_name("bw_res_t");
  gras_datadesc_type_t sat_request_desc;
  /* Build the saturation datatype descriptions */

  sat_request_desc = gras_datadesc_struct("s_sat_request_desc_t");
  gras_datadesc_struct_append(sat_request_desc, "peer",
                              gras_datadesc_by_name("s_xbt_peer_t"));
  gras_datadesc_struct_append(sat_request_desc, "msg_size",
                              gras_datadesc_by_name("unsigned int"));
  gras_datadesc_struct_append(sat_request_desc, "duration",
                              gras_datadesc_by_name("unsigned int"));
  gras_datadesc_struct_close(sat_request_desc);
  sat_request_desc = gras_datadesc_ref("sat_request_t", sat_request_desc);

  /* Register the saturation messages */
  gras_msgtype_declare_rpc("amok_bw_sat start", sat_request_desc, NULL);
  gras_msgtype_declare_rpc("amok_bw_sat begin", sat_request_desc,
                           sat_request_desc);
  gras_msgtype_declare_rpc("amok_bw_sat stop", NULL, bw_res_desc);

}

void amok_bw_sat_join(void)
{
  gras_cb_register("amok_bw_sat start", &amok_bw_cb_sat_start);
  gras_cb_register("amok_bw_sat begin", &amok_bw_cb_sat_begin);
}

void amok_bw_sat_leave(void)
{
  gras_cb_unregister("amok_bw_sat start", &amok_bw_cb_sat_start);
  gras_cb_unregister("amok_bw_sat begin", &amok_bw_cb_sat_begin);
}

/* ***************************************************************************
 * Link saturation
 * ***************************************************************************/

/**
 * @brief Ask 'from_name:from_port' to stop saturating going to to_name:to_name.
 *
 * @param from_name: Name of the peer we are asking to do a experiment with (to_name:to_port)
 * @param from_port: port on which the process we are asking for an experiment is listening
 * (for message, do not give a raw socket here. The needed raw socket will be negociated 
 * between the peers)
 * @param to_name: Name of the peer with which we should conduct the experiment
 * @param to_port: port on which the peer process is listening for message
 * @param msg_size: Size of each message sent.
 * @param duration: How long in maximum should be the saturation.
 *
 * Ask the process 'from_name:from_port' to start to saturate the link between itself
 * and to_name:to_name.
 */
void amok_bw_saturate_start(const char *from_name, unsigned int from_port,
                            const char *to_name, unsigned int to_port,
                            unsigned int msg_size, double duration)
{
  gras_socket_t sock;
  sat_request_t request = xbt_new(s_sat_request_t, 1);

  VERB4("Start from_name %s:%d -> to_name %s:%d",
        from_name, from_port, to_name, to_port);
  sock = gras_socket_client(from_name, from_port);

  request->peer.name = (char *) to_name;
  request->peer.port = to_port;

  request->duration = duration;
  request->msg_size = msg_size;

  gras_msg_rpccall(sock, 60, "amok_bw_sat start", &request, NULL);

  free(request);
  gras_socket_close(sock);
}

/* Asked to begin a saturation */
static int amok_bw_cb_sat_start(gras_msg_cb_ctx_t ctx, void *payload)
{
  sat_request_t request = *(sat_request_t *) payload;
  gras_socket_t expeditor = gras_msg_cb_ctx_from(ctx);

  VERB4("Asked by %s:%d to start a saturation to %s:%d",
        gras_socket_peer_name(expeditor), gras_socket_peer_port(expeditor),
        request->peer.name, request->peer.port);

  gras_msg_rpcreturn(60, ctx, NULL);

  amok_bw_saturate_begin(request->peer.name, request->peer.port,
                         request->msg_size, request->duration, NULL, NULL);

  free(request->peer.name);

  free(request);
  return 0;
}

/**
 * @brief Start saturating between the current process and the designated peer
 *
 * Note that the only way to break this function before the end of the timeout
 * is to have a remote peer calling amok_bw_saturate_stop to this process.
 *
 * If duration=0, the experiment will never timeout (you then have to manually
 * stop it).
 * 
 * If msg_size=0, the size will be automatically computed to make sure that
 * each of the messages occupy the connexion one second
 */
void amok_bw_saturate_begin(const char *to_name, unsigned int to_port,
                            unsigned int msg_size, double duration,
                            /*out */ double *elapsed_res, double *bw_res)
{

  xbt_ex_t e;

  gras_socket_t peer_cmd = gras_socket_client(to_name, to_port);
  gras_msg_cb_ctx_t ctx;

  gras_socket_t meas;

  s_gras_msg_t msg_got;

  unsigned int packet_sent = 0;
  double start, elapsed = -1;   /* timer */
  double bw;

  volatile int saturate_further;        /* boolean in the main loop */

  /* Negociate the saturation with the peer */
  sat_request_t request = xbt_new(s_sat_request_t, 1);

  DEBUG2("Begin to saturate to %s:%d", to_name, to_port);
  memset(&msg_got, 0, sizeof(msg_got));

  request->msg_size = msg_size;
  request->duration = duration;
  request->peer.name = NULL;
  request->peer.port = 0;


  /* Size autodetection on need */
  if (!msg_size) {
    double bw;
    double sec;
    amok_bw_test(peer_cmd, 0,   /* check buffsize yourself */
                 512 * 1024,    /* 512k as first guess */
                 1,             /* One packet only */
                 1,             /* at least one sec */
                 &sec, &bw);
    msg_size = request->msg_size = (int) bw;
    DEBUG1("Saturate with packets of %d bytes", request->msg_size);
  }

  /* Launch the saturation */

  ctx = gras_msg_rpc_async_call(peer_cmd, 60, "amok_bw_sat begin", &request);
  free(request);
  gras_msg_rpc_async_wait(ctx, &request);
  meas = gras_socket_client_ext(to_name, request->peer.port,
                                0 /*bufsize: auto */ ,
                                1 /*meas: true */ );

  free(request);

  gras_socket_close(peer_cmd);
  INFO4("Saturation(%s:%d->%s:%d) started", gras_os_myname(),
        gras_os_myport(), to_name, to_port);

  /* Start experiment */
  start = gras_os_time();

  do {
    /* do send it */
    gras_socket_meas_send(meas, 120, msg_size, 1);
    packet_sent++;

    /* Check whether someone asked us to stop saturation */
    saturate_further = 0;
    TRY {
      gras_msg_wait_ext(0 /*no wait */ , "amok_bw_sat stop",
                        NULL /* accept any sender */ ,
                        NULL, NULL,     /* No specific filter */
                        &msg_got);
    }
    CATCH(e) {
      if (e.category == timeout_error) {
        saturate_further = 1;
        memset(&msg_got, 0, sizeof(msg_got));   /* may be overprotectiv here */
      }
      xbt_ex_free(e);
    }

    /* Check whether the experiment has to be terminated by now */
    elapsed = gras_os_time() - start;
    DEBUG3("elapsed %f duration %f (msg_size=%d)", elapsed, duration,
           msg_size);

  } while (saturate_further && (duration == 0 || elapsed < duration));

  bw = ((double) (packet_sent * msg_size)) / elapsed;

  if (elapsed_res)
    *elapsed_res = elapsed;
  if (bw_res)
    *bw_res = bw;

  /* If someone stopped us, inform him about the achieved bandwidth */
  if (msg_got.expe) {
    bw_res_t answer = xbt_new(s_bw_res_t, 1);
    s_gras_msg_cb_ctx_t ctx;

    INFO6("Saturation from %s:%d to %s:%d stopped by %s:%d",
          gras_os_myname(), gras_os_myport(), to_name, to_port,
          gras_socket_peer_name(msg_got.expe),
          gras_socket_peer_port(msg_got.expe));
    answer->timestamp = gras_os_time();
    answer->sec = elapsed;
    answer->bw = bw;

    ctx.expeditor = msg_got.expe;
    ctx.ID = msg_got.ID;
    ctx.msgtype = msg_got.type;

    gras_msg_rpcreturn(60, &ctx, &answer);
    free(answer);
  } else {
    INFO6
      ("Saturation from %s:%d to %s:%d elapsed after %f sec (achieving %f kb/s)",
       gras_os_myname(), gras_os_myport(), to_name, to_port, elapsed,
       bw / 1024.0);
  }

  gras_socket_close(meas);
}

/* Sender will saturate link to us */
static int amok_bw_cb_sat_begin(gras_msg_cb_ctx_t ctx, void *payload)
{
  sat_request_t request = *(sat_request_t *) payload;
  sat_request_t answer = xbt_new0(s_sat_request_t, 1);
  volatile int saturate_further = 1;
  xbt_ex_t e;
  gras_socket_t measMaster = NULL, meas = NULL;
  gras_socket_t from = gras_msg_cb_ctx_from(ctx);

  int port = 6000;
  while (port <= 10000 && measMaster == NULL) {
    TRY {
      measMaster = gras_socket_server_ext(port, 0 /*bufsize: auto */ ,
                                          1 /*meas: true */ );
    }
    CATCH(e) {
      measMaster = NULL;
      if (port < 10000)
        xbt_ex_free(e);
      else
        RETHROW0
          ("Error encountered while opening a measurement server socket: %s");
    }
    if (measMaster == NULL)
      port++;                   /* prepare for a new loop */
  }
  answer->peer.port = port;

  gras_msg_rpcreturn(60, ctx, &answer);
  free(answer);

  gras_os_sleep(5);             /* Wait for the accept */

  TRY {
    meas = gras_socket_meas_accept(measMaster);
    DEBUG0("saturation handshake answered");
  }
  CATCH(e) {
    gras_socket_close(measMaster);
    RETHROW0("Error during saturation handshake: %s");
  }

  while (saturate_further) {
    TRY {
      gras_socket_meas_recv(meas, 5, request->msg_size, 1);
    }
    CATCH(e) {
      saturate_further = 0;
      xbt_ex_free(e);
    }
  }
  INFO4("Saturation comming from %s:%d stopped on %s:%d",
        gras_socket_peer_name(from), gras_socket_peer_port(from),
        gras_os_myname(), gras_os_myport());

  gras_socket_close(meas);
  if (gras_if_RL())             /* On SG, accepted=master */
    gras_socket_close(measMaster);
  free(request);
  return 0;
}

/**
 * @brief Ask 'from_name:from_port' to stop any saturation experiments
 * @param from_name: Name of the peer we are asking to do a experiment with (to_name:to_port)
 * @param from_port: port on which the process we are asking for an experiment is listening
 * @param time: the duration of the experiment
 * @param bw: the achieved bandwidth
 *
 */
void amok_bw_saturate_stop(const char *from_name, unsigned int from_port,
                           /*out */ double *time, double *bw)
{
  xbt_ex_t e;

  gras_socket_t sock = gras_socket_client(from_name, from_port);
  bw_res_t answer;
  VERB2("Ask %s:%d to stop the saturation", from_name, from_port);
  TRY {
    gras_msg_rpccall(sock, 60, "amok_bw_sat stop", NULL, &answer);
  } CATCH(e) {
    RETHROW2("Cannot ask %s:%d to stop saturation: %s", from_name, from_port);
  }
  gras_socket_close(sock);
  if (time)
    *time = answer->sec;
  if (bw)
    *bw = answer->bw;
  free(answer);
}
