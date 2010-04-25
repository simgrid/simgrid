/* saturate - link saturation demo of AMOK features                         */

/* Copyright (c) 2003-6 Martin Quinson. All rights reserved.                */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#include "xbt/peer.h"
#include "gras.h"
#include "amok/bandwidth.h"
#include "amok/peermanagement.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(saturate, "Messages specific to this example");

/* **********************************************************************
 * Sensor code
 * **********************************************************************/

/* Function prototypes */
int sensor(int argc, char *argv[]);

int sensor(int argc, char *argv[])
{
  gras_socket_t mysock;
  gras_socket_t master;

  gras_init(&argc, argv);
  amok_bw_init();
  amok_pm_init();

  mysock = gras_socket_server_range(3000, 9999, 0, 0);
  INFO1("Sensor starting (on port %d)", gras_os_myport());
  gras_os_sleep(2);             /* let the master get ready */
  master = gras_socket_client_from_string(argv[1]);

  amok_pm_group_join(master, "saturate");
  amok_pm_mainloop(600);

  gras_socket_close(mysock);
  gras_socket_close(master);
  gras_exit();
  return 0;
}

/* **********************************************************************
 * Maestro code
 * **********************************************************************/

/* Function prototypes */
int maestro(int argc, char *argv[]);

/* XP setups */
const int buf_size = 0;
const int msg_size = 50 * 1024;
const int msg_amount = 2;
const int sat_size = 1024 * 1024 * 10;
const double min_duration = 1;

static double XP(const char *bw1, const char *bw2,
                 const char *sat1, const char *sat2)
{

  double sec, bw, sec_sat, bw_sat;

  gras_os_sleep(5.0);           /* wait for the sensors to show up */
  /* Test BW without saturation */
  amok_bw_request(bw1, 4000, bw2, 4000,
                  buf_size, msg_size, msg_amount, min_duration, &sec, &bw);
  INFO4("BW(%s,%s) => %f sec, achieving %f Mb/s",
        bw1, bw2, sec, (bw / 1024.0 / 1024.0));


  /* Test BW with saturation */
  amok_bw_saturate_start(sat1, 4000, sat2, 4000, sat_size, 60);
  gras_os_sleep(1.0);           /* let it start */

  amok_bw_request(bw1, 4000, bw2, 4000,
                  buf_size, msg_size, msg_amount, min_duration, &sec_sat,
                  &bw_sat);
  INFO6("BW(%s,%s//%s,%s) => %f sec, achieving %f Mb/s", bw1, bw2, sat1, sat2,
        sec, bw / 1024.0 / 1024.0);

  amok_bw_saturate_stop(sat1, 4000, NULL, NULL);

  if (bw_sat / bw < 0.7) {
    INFO0("THERE IS SOME INTERFERENCE !!!");
  }
  if (bw / bw_sat < 0.7) {
    INFO0("THERE IS SOME INTERFERENCE (and I'm an idiot) !!!");
  }
  return bw_sat / bw;
}

static void kill_buddy(char *name, int port)
{
  gras_socket_t sock = gras_socket_client(name, port);
  gras_msg_send(sock, "kill", NULL);
  gras_socket_close(sock);
}

static void kill_buddy_dynar(void *b)
{
  xbt_peer_t buddy = *(xbt_peer_t *) b;
  kill_buddy(buddy->name, buddy->port);
}

static void free_peer(void *d)
{
  xbt_peer_t h = *(xbt_peer_t *) d;
  free(h->name);
  free(h);
}

static void simple_saturation(int argc, char *argv[])
{
  xbt_ex_t e;

  /* where are the sensors */
  xbt_dynar_t peers;
  xbt_peer_t h1, h2;
  /* results */
  double duration, bw;

  /* Init the group */
  peers = amok_pm_group_new("saturate");
  /* wait for dudes */
  gras_msg_handleall(5);

  /* Stop all sensors but two of them */
  while (xbt_dynar_length(peers) > 2) {
    xbt_dynar_pop(peers, &h1);
    amok_pm_kill_hp(h1->name, h1->port);
    xbt_peer_free(h1);
  }

  /* get 2 friends */
  xbt_dynar_get_cpy(peers, 0, &h1);
  xbt_dynar_get_cpy(peers, 1, &h2);

  /* Start saturation */
  INFO4("Start saturation between %s:%d and %s:%d",
        h1->name, h1->port, h2->name, h2->port);

  amok_bw_saturate_start(h1->name, h1->port, h2->name, h2->port, 0,     /* Be a nice boy, compute msg_size yourself */
                         30 /* 5 sec timeout */ );

  /* Stop it after a while */
  INFO0("Have a rest");
  gras_os_sleep(1);
  TRY {
    INFO0("Stop the saturation");
    amok_bw_saturate_stop(h1->name, h1->port, &duration, &bw);
  }
  CATCH(e) {
    INFO0("Ooops, stoping the saturation raised an exception");
    xbt_ex_free(e);
  }
  INFO2("Saturation took %.2fsec, achieving %fb/s", duration, bw);

  /* Game is over, friends */
  amok_pm_group_shutdown("saturate");
}

/********************************************************************************************/
static void full_fledged_saturation(int argc, char *argv[])
{
  xbt_ex_t e;
  double time1 = 5.0, bw1 = 5.0;        // 0.5 for test
  /* timers */
  double begin_simulated;
  int begin;

  /* where are the sensors */
  xbt_dynar_t peers;
  int nb_peers;

  /* results */
  double *bw;
  double *bw_sat;

  /* iterators */
  unsigned int i, j, k, l;
  xbt_peer_t h1, h2, h3, h4;

  /* Init the group */
  peers = amok_pm_group_new("saturate");
  /* wait 4 dudes */
  gras_msg_handle(60);
  gras_msg_handle(60);
  gras_msg_handle(60);
  gras_msg_handle(60);
  nb_peers = xbt_dynar_length(peers);

  INFO0("Let's go for the bw_matrix");

  /* Do the test without saturation */
  begin = time(NULL);
  begin_simulated = gras_os_time();

  bw = amok_bw_matrix(peers, buf_size, msg_size, msg_amount, min_duration);

  INFO2("Did all BW tests in %ld sec (%.2f simulated(?) sec)",
        (long int) (time(NULL) - begin), gras_os_time() - begin_simulated);

  /* Do the test with saturation */
  bw_sat = xbt_new(double, nb_peers * nb_peers);
  xbt_dynar_foreach(peers, i, h1) {
    xbt_dynar_foreach(peers, j, h2) {
      if (i == j)
        continue;

      TRY {
        amok_bw_saturate_start(h1->name, h1->port, h2->name, h2->port, 0,       /* Be nice, compute msg_size yourself */
                               0 /* no timeout */ );
      }
      CATCH(e) {
        RETHROW0("Cannot ask peers to saturate the link: %s");
      }
      gras_os_sleep(5);

      begin = time(NULL);
      begin_simulated = gras_os_time();
      xbt_dynar_foreach(peers, k, h3) {
        if (i == k || j == k)
          continue;

        xbt_dynar_foreach(peers, l, h4) {
          double ratio;
          if (i == l || j == l || k == l)
            continue;

          VERB4("TEST %s %s // %s %s",
                h1->name, h2->name, h3->name, h4->name);
          amok_bw_request(h3->name, h3->port, h4->name, h4->port,
                          buf_size, msg_size, msg_amount, min_duration,
                          NULL, &(bw_sat[k * nb_peers + l]));

          ratio = bw_sat[k * nb_peers + l] / bw[k * nb_peers + l];
          INFO8("SATURATED BW XP(%s %s // %s %s) => %f (%f vs %f)%s",
                h1->name, h2->name, h3->name, h4->name,
                ratio,
                bw[k * nb_peers + l], bw_sat[k * nb_peers + l],
                ratio < 0.7 ? " THERE IS SOME INTERFERENCE !!!" : "");
        }
      }
      amok_bw_saturate_stop(h1->name, h1->port, &time1, &bw1);

      INFO2
        ("Did an iteration on saturation pair in %ld sec (%.2f simulated sec)",
         (long int) (time(NULL) - begin), gras_os_time() - begin_simulated);
      INFO2("the duration of the experiment >>>>> %.3f sec (%.3f bandwidth)",
            time1, bw1);
    }
  }
  free(bw_sat);
  free(bw);
  /* Game is over, friends */
  amok_pm_group_shutdown("saturate");
}


int maestro(int argc, char *argv[])
{

  gras_init(&argc, argv);
  amok_bw_init();
  amok_pm_init();

  gras_socket_server(atoi(argv[1]));

  simple_saturation(argc, argv);
  //full_fledged_saturation(argc, argv);  

  gras_exit();
  return 0;

}
