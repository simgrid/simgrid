/* $Id$ */

/* bandwidth - bandwidth test demo of GRAS features                         */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <gras.h>

/* **********************************************************************
 * Sensor code
 * **********************************************************************/

/* Global private data */
typedef struct {
  gras_sock_t *sock;
} sensor_data_t;

/* Function prototypes */
int sensor (int argc,char *argv[]);

int sensor (int argc,char *argv[]) {
  gras_error_t errcode;
  sensor_data_t *g=gras_userdata_new(sensor_data_t);  

  if ((errcode=gras_sock_server_open(4000,4000,&(g->sock)))) { 
    fprintf(stderr,"Sensor: Error %s encountered while opening the server socket\n",gras_error_name(errcode));
    return 1;
  }

  if (grasbw_register_messages()) {
    gras_sock_close(g->sock);
    return 1;
  }

  while (1) {
    if ((errcode=gras_msg_handle(60.0)) && errcode != timeout_error) {
      fprintf(stderr,"Sensor: Error '%s' while handling message\n",gras_error_name(errcode));
      gras_sock_close(g->sock);
      return errcode;
    }
    if (errcode==no_error)
       break;
  }

  gras_sleep(5,0);
  return gras_sock_close(g->sock);
}

/* **********************************************************************
 * Maestro code
 * **********************************************************************/

/* Global private data */
typedef struct {
  gras_sock_t *sock;
} maestro_data_t;

/* Function prototypes */
int maestro (int argc,char *argv[]);

int maestro(int argc,char *argv[]) {
  gras_error_t errcode;
  maestro_data_t *g=gras_userdata_new(maestro_data_t);
  double sec, bw;
  int bufSize=32 * 1024;
  int expSize=64 * 1024;
  int msgSize=64 * 1024;

  if ((errcode=gras_sock_server_open(4000,5000,&(g->sock)))) { 
    fprintf(stderr,"Maestro: Error %s encountered while opening the server socket\n",gras_error_name(errcode));
    return 1;
  }

  if (grasbw_register_messages()) {
    gras_sock_close(g->sock);
    return 1;
  }

  if (argc != 5) {
     fprintf(stderr,"Usage: maestro host port host port\n");
     return 1;
  }
     
  if ((errcode=grasbw_request(argv[1],atoi(argv[2]),argv[3],atoi(argv[4]),
			      bufSize,expSize,msgSize,&sec,&bw))) {
    fprintf(stderr,"maestro: Error %s encountered while doing the test\n",gras_error_name(errcode));
    return 1;
  }
   
  fprintf(stderr,"maestro: Experience (%d ko in msgs of %d ko) took %f sec, achieving %f Mb/s\n",
	  expSize/1024,msgSize/1024,
	  sec,bw);

  gras_sleep(5,0);
  return gras_sock_close(g->sock);
}
