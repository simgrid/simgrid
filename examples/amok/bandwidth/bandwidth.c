/* $Id$ */

/* bandwidth - bandwidth test demo of GRAS features                         */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include "gras.h"
#include "amok/bandwidth.h"
#include "amok/peermanagement.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(Bandwidth,"Messages specific to this example");

/* **********************************************************************
 * Sensor code
 * **********************************************************************/

/* Function prototypes */
int sensor (int argc,char *argv[]);

int sensor (int argc,char *argv[]) {
  gras_socket_t mysock;
  gras_socket_t master;

  gras_init(&argc, argv);
  amok_bw_init();
  amok_pm_init();
 
  mysock = gras_socket_server_range(3000,9999,0,0);
  INFO1("Sensor starting (on port %d)",gras_os_myport());
  gras_os_sleep(0.5); /* let the master get ready */
  master = gras_socket_client_from_string(argv[1]);
					      
  amok_pm_group_join(master,"bandwidth");
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
int maestro (int argc,char *argv[]);

int maestro(int argc,char *argv[]) {
  double sec, bw;
  int buf_size=32  *1024;
  int exp_size=512 *1024;
  int msg_size=512 *1024;
  double min_duration = 1;
   
  gras_socket_t peer;
  gras_socket_t mysock;
  xbt_peer_t h1,h2;
  xbt_dynar_t group;

  gras_init(&argc, argv);
  amok_bw_init();
  amok_pm_init();

  INFO0("Maestro starting");
  if (argc != 2) {
     ERROR0("Usage: maestro port\n");
     return 1;
  }
  mysock=gras_socket_server(atoi(argv[1]));
  group=amok_pm_group_new("bandwidth");
  INFO0("Wait for peers for 5 sec");
  gras_msg_handleall(5); /* friends, we're ready. Come and play */
   
  if (xbt_dynar_length(group) < 2) {
     char *msg;
     asprintf(&msg,"Not enough peers arrived. Expected 2 got %ld",
	      xbt_dynar_length(group));
     xbt_die(msg);
  }
  h1 = *(xbt_peer_t*) xbt_dynar_get_ptr(group, 0);
  h2 = *(xbt_peer_t*)xbt_dynar_get_ptr(group, 1);

  INFO2("Contact %s:%d",h1->name, h1->port);
  peer = gras_socket_client(h1->name, h1->port);

  INFO0("Test the BW between me and one of the sensors");  
  amok_bw_test(peer,buf_size,exp_size,msg_size,min_duration,&sec,&bw);
  INFO6("Experience between me and %s:%d (%d bytes in msgs of %d bytes) took %f sec, achieving %f kb/s",
	h1->name, h1->port,
	exp_size,msg_size,
	sec,((double)bw)/1024.0);

  INFO4("Test the BW between %s:%d and %s:%d",	h1->name, h1->port,	h2->name, h2->port);
  amok_bw_request(h1->name, h1->port,	h2->name, h2->port,
		  buf_size,exp_size,msg_size,min_duration,&sec,&bw);
  INFO6("Experience between %s:%d and %s:%d took took %f sec, achieving %f kb/s",
	h1->name, h1->port,	h2->name, h2->port,
	sec,((double)bw)/1024.0);

  /* Game is over, friends */
  amok_pm_group_shutdown ("bandwidth");

  gras_socket_close(mysock);
  gras_exit();
  return 0;
}
