/* saturate - link saturation demo of AMOK features                         */

/* Copyright (c) 2006, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#include "gras.h"
#include "amok/bandwidth.h"
#include "amok/hostmanagement.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(saturate,
                             "Messages specific to this example");

static void env_hosttohost_bw(int argc, char *argv[])
{

  /* where are the sensors */
  xbt_dynar_t hosts = xbt_dynar_new(sizeof(xbt_host_t), &free_host);
  int nb_hosts;

  /* results */
  double sec, bw;

  /* iterators */
  int i;
  xbt_host_t h1;

  gras_socket_t peer;           /* socket to sensor */

  /* wait to ensure that all server sockets are there before starting the experiment */
  gras_os_sleep(0.5);

  /* Get the sensor location from argc/argv */
  for (i = 1; i < argc - 1; i += 2) {
    xbt_host_t host = xbt_new(s_xbt_host_t, 1);
    host->name = strdup(argv[i]);
    host->port = atoi(argv[i + 1]);
    INFO2("New sensor: %s:%d", host->name, host->port);
    xbt_dynar_push(hosts, &host);
  }
  nb_hosts = xbt_dynar_length(hosts);

  INFO0(">>> start Test1: ENV end to end mesurements");

  xbt_dynar_foreach(hosts, i, h1) {
    peer = gras_socket_client(h1->name, h1->port);
    amok_bw_test(peer, buf_size, exp_size, msg_size, min_duration, &sec,
                 &bw);
    INFO6
        ("Bandwidth between me and %s:%d (%d bytes in msgs of %d bytes) took %f sec, achieving %.3f kb/s",
         h1->name, h1->port, exp_size, msg_size, sec,
         ((double) bw) / 1024.0);
  }

  xbt_dynar_map(hosts, kill_buddy_dynar);
  xbt_dynar_free(&hosts);

}

/********************************************************************************************/
static void env_Pairwisehost_bw(int argc, char *argv[])
{
  xbt_ex_t e;

  /* where are the sensors */
  xbt_dynar_t hosts = xbt_dynar_new(sizeof(xbt_host_t), &free_host);
  int nb_hosts;

  /* getting the name of maestro for the saturation and the concurrent bandwidth measurements  */
  char *host_test = argv[0];

  /* results */
  double sec, bw;

  /* iterators */
  int i, j;
  xbt_host_t h1, h2;

  /* socket to sensor */
  gras_socket_t peer;

  /* wait to ensure that all server sockets are there before starting the experiment */
  gras_os_sleep(0.5);

  INFO1(">>>>>< le maestro est: %s ", argv[0]);
  /* Get the sensor location from argc/argv */
  for (i = 1; i < argc - 1; i += 2) {
    xbt_host_t host = xbt_new(s_xbt_host_t, 1);
    host->name = strdup(argv[i]);
    host->port = atoi(argv[i + 1]);
    INFO2("New sensor: %s:%d", host->name, host->port);
    xbt_dynar_push(hosts, &host);
  }
  nb_hosts = xbt_dynar_length(hosts);

  INFO0(">>> start Test2: ENV pairwise host bandwidth mesurements");
  xbt_dynar_foreach(hosts, i, h1) {

    TRY {
      amok_bw_saturate_start(h1->name, h1->port, host_test, h1->port,   //"Ginette"
                             msg_size, 120);    // sturation of the link with msg_size to compute a concurent bandwidth MA //MB
    }
    CATCH(e) {
      RETHROW0("Cannot ask hosts to saturate the link: %s");
    }
    // gras_os_sleep(1.0);

    xbt_dynar_foreach(hosts, j, h2) {
      if (i == j)
        continue;

      peer = gras_socket_client(h2->name, h2->port);
      amok_bw_test(peer, buf_size, exp_size, msg_size, min_duration, &sec,
                   &bw);
      INFO6
          ("Bandwidth between me and %s // measurement between me and %s (%d bytes in msgs of %d bytes) took %f sec, achieving %.3f kb/s",
           h2->name, h1->name, exp_size, msg_size, sec,
           ((double) bw) / 1024.0);

    }
    amok_bw_saturate_stop(h1->name, h1->port, NULL, NULL);
  }
  xbt_dynar_map(hosts, kill_buddy_dynar);
  xbt_dynar_free(&hosts);

}

int maestro(int argc, char *argv[])
{

  gras_init(&argc, argv);
  amok_bw_init();
  amok_hm_init();

  gras_socket_server(3333);     /* only so that messages from the transport layer in gras identify us */

  //env_Pairwisehost_bw(argc,argv);
  //env_hosttohost_bw(argc,argv);

  gras_exit();
  return 0;

}
