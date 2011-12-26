/* bandwidth - bandwidth test demo of GRAS features                         */

/* Copyright (c) 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include "gras.h"
#include "amok/bandwidth.h"
#include "amok/peermanagement.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(Bandwidth,
                             "Messages specific to this example");

/* **********************************************************************
 * Sensor code
 * **********************************************************************/

static gras_socket_t try_gras_socket_client_from_string(const char *host)
{
  volatile gras_socket_t sock = NULL;
  xbt_ex_t e;
  TRY {
    sock = gras_socket_client_from_string(host);
  }
  CATCH(e) {
    xbt_ex_free(e);
  }
  return sock;
}

/* Function prototypes */
int sensor(int argc, char *argv[]);

int sensor(int argc, char *argv[])
{
  gras_socket_t mysock;
  gras_socket_t master = NULL;
  int connection_try = 10;

  gras_init(&argc, argv);
  amok_bw_init();
  amok_pm_init();

  mysock = gras_socket_server_range(3000, 9999, 0, 0);
  XBT_INFO("Sensor starting (on port %d)", gras_os_myport());
  while (connection_try > 0 &&
         !(master = try_gras_socket_client_from_string(argv[1]))) {
    connection_try--;
    gras_os_sleep(0.5);       /* let the master get ready */
  }

  amok_pm_group_join(master, "bandwidth");
  amok_pm_mainloop(60);

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

int maestro(int argc, char *argv[])
{
  double sec, bw;
  int buf_size = 32 * 1024;
  int msg_size = 512 * 1024;
  int msg_amount = 1;
  double min_duration = 1;

  gras_socket_t peer;
  gras_socket_t mysock;
  xbt_peer_t h1, h2, h_temp;
  xbt_dynar_t group;

  gras_init(&argc, argv);
  amok_bw_init();
  amok_pm_init();

  XBT_INFO("Maestro starting");
  if (argc != 2) {
    XBT_ERROR("Usage: maestro port\n");
    return 1;
  }
  mysock = gras_socket_server(atoi(argv[1]));
  group = amok_pm_group_new("bandwidth");
  XBT_INFO("Wait for peers for 5 sec");
  gras_msg_handleall(5);        /* friends, we're ready. Come and play */

  if (xbt_dynar_length(group) < 2) {
    amok_pm_group_shutdown("bandwidth");
    xbt_die("Not enough peers arrived. Expected 2 got %ld",
            xbt_dynar_length(group));
  }
  h1 = *(xbt_peer_t *) xbt_dynar_get_ptr(group, 0);
  h2 = *(xbt_peer_t *) xbt_dynar_get_ptr(group, 1);
  /* sort peers in right order to keep output right */
  if (strcmp(h1->name, h2->name) < 0 || h1->port > h2->port) {
    h_temp = h1;
    h1 = h2;
    h2 = h_temp;
  }

  XBT_INFO("Contact %s:%d", h1->name, h1->port);
  peer = gras_socket_client(h1->name, h1->port);

  XBT_INFO("Test the BW between me and one of the sensors");
  amok_bw_test(peer, buf_size, msg_size, msg_amount, min_duration, &sec,
               &bw);
  XBT_INFO
      ("Experience between me and %s:%d (initially %d msgs of %d bytes, maybe modified to fill the pipe at least %.1fs) took %f sec, achieving %f kb/s",
       h1->name, h1->port, msg_amount, msg_size, min_duration, sec,
       ((double) bw) / 1024.0);

  XBT_INFO("Test the BW between %s:%d and %s:%d", h1->name, h1->port,
        h2->name, h2->port);
  amok_bw_request(h1->name, h1->port, h2->name, h2->port, buf_size,
                  msg_size, msg_amount, min_duration, &sec, &bw);
  XBT_INFO
      ("Experience between %s:%d and %s:%d took took %f sec, achieving %f kb/s",
       h1->name, h1->port, h2->name, h2->port, sec,
       ((double) bw) / 1024.0);

  /* Game is over, friends */
  amok_pm_group_shutdown("bandwidth");

  gras_socket_close(mysock);
  gras_exit();
  return 0;
}
