/* $Id$ */

/* saturate - link saturation demo of AMOK features                         */

/* Copyright (c) 2003-6 Martin Quinson. All rights reserved.                */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#include "gras.h"
#include "amok/bandwidth.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(saturate,"Messages specific to this example");

/* **********************************************************************
 * Sensor code
 * **********************************************************************/

/* Global private data */
typedef struct {
  gras_socket_t sock;
} sensor_data_t;

/* Function prototypes */
int sensor (int argc,char *argv[]);

int sensor (int argc,char *argv[]) {
  sensor_data_t *g;

  gras_init(&argc,argv);
  amok_bw_init();

  g=gras_userdata_new(sensor_data_t);  
  g->sock = gras_socket_server(4000);

  while (1) {
    gras_msg_handle(60.0);
  }

  return 0;
}

/* **********************************************************************
 * Maestro code
 * **********************************************************************/

/* Function prototypes */
int maestro (int argc,char *argv[]);

static double XP(const char *bw1, const char *bw2,
		 const char *sat1, const char *sat2) {

  int buf_size=32 * 1024;
  int exp_size=64 * 1024;
  int msg_size=64 * 1024;
  int sat_size=msg_size * 10;
  double sec, bw, sec_sat,bw_sat;

  /* Test BW without saturation */
  amok_bw_request(bw1,4000,bw2,4000,
		  buf_size,exp_size,msg_size,&sec,&bw);
  INFO4("BW(%s,%s) => %f sec, achieving %f Mb/s",
       bw1, bw2, sec, (bw/1024.0/1024.0));


  /* Test BW with saturation */  
  amok_bw_saturate_start(sat1,4000,sat2,4000, sat_size,60);
  gras_os_sleep(1.0); /* let it start */

  amok_bw_request(bw1,4000,bw2,4000,
		  buf_size,exp_size,msg_size,&sec_sat,&bw_sat);
  INFO6("BW(%s,%s//%s,%s) => %f sec, achieving %f Mb/s",
       bw1,bw2,sat1,sat2,sec,bw/1024.0/1024.0);
  
  amok_bw_saturate_stop(sat1,4000,NULL,NULL);

  if (bw_sat/bw < 0.7) {
    INFO0("THERE IS SOME INTERFERENCE !!!");
  } 
  if (bw/bw_sat < 0.7) {
    INFO0("THERE IS SOME INTERFERENCE (and I'm an idiot) !!!");
  } 
  return bw_sat/bw;
}

static void free_host(void *d){
  xbt_host_t h=*(xbt_host_t*)d;
  free(h->name);
  free(h);
}
int maestro(int argc,char *argv[]) {
  xbt_ex_t e;
  /* XP setups */
  int buf_size=0;
  int exp_size= 1024 * 1024;
  int msg_size=exp_size;
  int sat_size=msg_size * 100;

  /* timers */
  double begin_simulated; 
  int begin;

  /* where are the sensors */
  xbt_dynar_t hosts = xbt_dynar_new(sizeof(xbt_host_t),&free_host);
  int nb_hosts;

  /* results */
  double *bw;
  double *bw_sat;

  /* iterators */
  int i,j,k,l;
  xbt_host_t h1,h2,h3,h4;

  gras_init(&argc,argv);
  amok_bw_init();

  /* Get the sensor location from argc/argv */
  for (i=1; i<argc-1; i+=2){
    xbt_host_t host=xbt_new(s_xbt_host_t,1);
    host->name=strdup(argv[i]);
    host->port=atoi(argv[i+1]);
    INFO2("New sensor: %s:%d",host->name,host->port);
    xbt_dynar_push(hosts,&host);
  }
  nb_hosts = xbt_dynar_length(hosts);

  /* Do the test without saturation */
  begin=time(NULL);
  begin_simulated=gras_os_time();

  bw=amok_bw_matrix(hosts,buf_size,exp_size,msg_size);

  INFO2("Did all BW tests in %ld sec (%.2f simulated sec)",
	  time(NULL)-begin,gras_os_time()-begin_simulated);

  /* Do the test with saturation */
  bw_sat=xbt_new(double,nb_hosts*nb_hosts);
  xbt_dynar_foreach(hosts,i,h1) {
    xbt_dynar_foreach(hosts,j,h2) {
      if (i==j) continue;
      	
      TRY {
	amok_bw_saturate_start(h1->name,h1->port,
			       h2->name,h2->port,
			       sat_size,360000000);
      } CATCH(e) {
	RETHROW0("Cannot ask hosts to saturate the link: %s");
      }
      gras_os_sleep(1.0);

      begin=time(NULL);
      begin_simulated=gras_os_time();
      xbt_dynar_foreach(hosts,k,h3) {
	if (i==k || j==k) continue;

	xbt_dynar_foreach(hosts,l,h4) {
	  double ratio;
	  if (i==l || j==l || k==l) continue;

	  amok_bw_request(h3->name,h3->port, h4->name,h4->port,
			  buf_size,exp_size,msg_size,
			  NULL,&(bw_sat[k*nb_hosts + l])); 
	  
	  ratio=bw_sat[k*nb_hosts + l] / bw[k*nb_hosts + l];
	  INFO8("SATURATED BW XP(%s %s // %s %s) => %f (%f vs %f)%s",
		h1->name,h2->name,h3->name,h4->name,
		ratio,
		bw[k*nb_hosts + l] , bw_sat[k*nb_hosts + l],

		ratio < 0.7 ? " THERE IS SOME INTERFERENCE !!!": "");
	}
      }

      amok_bw_saturate_stop(h1->name,h1->port, NULL,NULL);

      INFO2("Did an iteration on saturation pair in %ld sec (%.2f simulated sec)",
	      time(NULL)-begin, gras_os_time()-begin_simulated);
    }
  }

  gras_os_sleep(5.0);
  exit(0);
#if 0
  return 0;
  /* start saturation */
  fprintf(stderr,"MAESTRO: Start saturation with size %d",msg_size);
  if ((errcode=grasbw_saturate_start(argv[5],atoi(argv[6]),argv[7],atoi(argv[8]),msg_size*10,60))) {
    fprintf(stderr,"MAESTRO: Error %s encountered while starting saturation",
	    xbt_error_name(errcode));
    return 1;
  }
  fprintf(stderr,"MAESTRO: Saturation started");
  gras_os_sleep(5.0);

  /* test with saturation */
  if ((errcode=grasbw_request(argv[1],atoi(argv[2]),argv[3],atoi(argv[4]),
			      buf_size,exp_size,msg_size,&sec,&bw))) {
    fprintf(stderr,"MAESTRO: Error %s encountered while doing the test",xbt_error_name(errcode));
    return 1;
  }
   
  fprintf(stderr,"MAESTRO: Experience3 (%d ko in msgs of %d ko with saturation) took %f sec, achieving %f Mb/s",
	  exp_size/1024,msg_size/1024,
	  sec,bw);

  /* stop saturation */
  if ((errcode=grasbw_saturate_stop(argv[5],atoi(argv[6]),argv[7],atoi(argv[8])))) {
    fprintf(stderr,"MAESTRO: Error %s encountered while stopping saturation",
	    xbt_error_name(errcode));
    return 1;
  }

  /* test without saturation */
  if ((errcode=grasbw_request(argv[1],atoi(argv[2]),argv[3],atoi(argv[4]),
			      buf_size,exp_size,msg_size,&sec,&bw))) {
    fprintf(stderr,"MAESTRO: Error %s encountered while doing the test",xbt_error_name(errcode));
    return 1;
  }
   
  fprintf(stderr,"MAESTRO: Experience4 (%d ko in msgs of %d ko, without saturation) took %f sec, achieving %f Mb/s",
	  exp_size/1024,msg_size/1024,
	  sec,bw);

  gras_os_sleep(5.0);
#endif
   
  return 0;
}
