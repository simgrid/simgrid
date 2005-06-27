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
  int done;
} s_sensor_data_t,*sensor_data_t;

static int sensor_cb_quit(gras_socket_t  expeditor,
                          void          *payload_data) {
  sensor_data_t globals=(sensor_data_t)gras_userdata_get();
                          
  globals->done = 1;                  
  return 1;         
}

/* Function prototypes */
int sensor (int argc,char *argv[]);

int sensor (int argc,char *argv[]) {
  xbt_error_t errcode;
  sensor_data_t g;

  gras_init(&argc, argv);
  g=gras_userdata_new(s_sensor_data_t);  
  amok_bw_init();
   
  if ((errcode=gras_socket_server(atoi(argv[1]),&(g->sock)))) { 
    ERROR1("Error %s encountered while opening the server socket",xbt_error_name(errcode));
    return 1;
  }
  g->done = 0;
  
  gras_msgtype_declare("quit",NULL);
  gras_cb_register(gras_msgtype_by_name("quit"),&sensor_cb_quit);
  
  while (! g->done ) {
    errcode=gras_msg_handle(60.0);
    if (errcode != no_error) {
       ERROR1("Error '%s' while handling message",xbt_error_name(errcode));
       return errcode;
    }	
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
  int exp_size=1024*50;
  int msg_size=512;
  gras_socket_t peer;

  gras_init(&argc, argv);
  g=gras_userdata_new(s_maestro_data_t);
  amok_bw_init();

  if (argc != 5) {
     ERROR0("Usage: maestro host port host port\n");
     return 1;
  }

  /* wait to ensure that all server sockets are there before starting the experiment */	
  gras_os_sleep(0.5);
  
  if ((errcode=gras_socket_client(argv[1],atoi(argv[2]),&peer))) {
     ERROR3("Unable to connect to my peer on %s:%s. Got %s",
	    argv[1],argv[2],xbt_error_name(errcode));
     return 1;
  }

  INFO0("Test the BW between me and one of the sensors");  
  TRY(amok_bw_test(peer,buf_size,exp_size,msg_size,&sec,&bw));
  INFO6("Experience between me and %s:%d (%d kb in msgs of %d kb) took %f sec, achieving %f kb/s",
	argv[1],atoi(argv[2]),
	exp_size,msg_size,
	sec,bw);

  INFO0("Test the BW between the two sensors");  
  TRY(amok_bw_request(argv[1],atoi(argv[2]),argv[3],atoi(argv[4]),
                      buf_size,exp_size,msg_size,&sec,&bw));	
  INFO2("Experience took took %f sec, achieving %f kb/s",
	sec,bw);

  /* ask sensors to quit */                    
  gras_msgtype_declare("quit",NULL);
  TRY(gras_msg_send(peer,gras_msgtype_by_name("quit"), NULL));
  gras_socket_close(peer);
  TRY(gras_socket_client(argv[3],atoi(argv[4]),&peer));
  TRY(gras_msg_send(peer,gras_msgtype_by_name("quit"), NULL));
  gras_socket_close(peer);

  gras_socket_close(g->sock);
  return 0;
}
