/* $Id$ */

/* bandwidth - bandwidth test demo of GRAS features                         */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include "gras.h"
#include "amok/bandwidth.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(Bandwidth,"Messages specific to this example");

/* **********************************************************************
 * Sensor code
 * **********************************************************************/

/* Global private data */
typedef struct {
  gras_socket_t sock;
} s_sensor_data_t,*sensor_data_t;

/* Function prototypes */
int sensor (int argc,char *argv[]);

int sensor (int argc,char *argv[]) {
  xbt_error_t errcode;
  sensor_data_t g;

  gras_init(&argc, argv, NULL);
  g=gras_userdata_new(s_sensor_data_t);  

  amok_bw_init();
   
  if ((errcode=gras_socket_server(atoi(argv[1]),&(g->sock)))) { 
    ERROR1("Sensor: Error %s encountered while opening the server socket",xbt_error_name(errcode));
    return 1;
  }

  errcode=gras_msg_handle(60.0);
  if (errcode != no_error) {
     ERROR1("Sensor: Error '%s' while handling message",xbt_error_name(errcode));
     gras_socket_close(g->sock);
     return errcode;
  }

  gras_socket_close(g->sock);
  return 0;
}

/* **********************************************************************
 * Maestro code
 * **********************************************************************/

/* Global private data */
typedef struct {
  gras_socket_t sock;
} s_maestro_data_t,*maestro_data_t;

/* Function prototypes */
int maestro (int argc,char *argv[]);

int maestro(int argc,char *argv[]) {
  xbt_error_t errcode;
  maestro_data_t g;
  double sec, bw;
  int buf_size=32;
//  int exp_size=64;
  int exp_size=1024 * 1024;
  int msg_size=1024;
//  int msg_size=64;
  gras_socket_t peer;

  gras_init(&argc, argv, NULL);
  g=gras_userdata_new(s_maestro_data_t);
  amok_bw_init();
   
  if ((errcode=gras_socket_server(6000,&(g->sock)))) { 
    ERROR1("Maestro: Error %s encountered while opening the server socket",xbt_error_name(errcode));
    return 1;
  }
      
   
  if (argc != 5) {
     ERROR0("Usage: maestro host port host port\n");
     return 1;
  }

  /* wait to ensure that all server sockets are there before starting the experiment */	
  gras_os_sleep(1.0);
  
  if ((errcode=gras_socket_client(argv[1],atoi(argv[2]),&peer))) {
     ERROR3("Client: Unable to connect to my peer on %s:%s. Got %s",
	    argv[1],argv[2],xbt_error_name(errcode));
     return 1;
  }

/*  if ((errcode=amok_bw_request(argv[1],atoi(argv[2]),argv[3],atoi(argv[4]),
			       buf_size,exp_size,msg_size,&sec,&bw))) {*/
  
  if ((errcode=amok_bw_test(peer,buf_size,exp_size,msg_size,&sec,&bw))) {
    ERROR1("maestro: Error %s encountered while doing the test",xbt_error_name(errcode));
    return 1;
  }
   
  INFO6("maestro: Experience between me and %s:%d (%d kb in msgs of %d kb) took %f sec, achieving %f kb/s",
	argv[1],atoi(argv[2]),
	exp_size,msg_size,
	sec,bw);

  gras_socket_close(g->sock);
  return 0;
}
