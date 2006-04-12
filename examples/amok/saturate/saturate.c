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
  int done;
} sensor_data_t;

/* Function prototypes */
int sensor (int argc,char *argv[]);

static int sensor_cb_kill(gras_msg_cb_ctx_t ctx,
			  void             *payload_data) {
  
  sensor_data_t *g=gras_userdata_get();
  g->done = 1;
  INFO0("Killed");
  return 1;
}
int sensor (int argc,char *argv[]) {
  sensor_data_t *g;

  gras_init(&argc,argv);
  amok_bw_init();

  g=gras_userdata_new(sensor_data_t);  
  g->sock = gras_socket_server(4000);
  g->done = 0;
  gras_msgtype_declare("kill",NULL);
  gras_cb_register(gras_msgtype_by_name("kill"),&sensor_cb_kill);

  while (!g->done) {
    gras_msg_handle(120.0);
  }

  gras_socket_close(g->sock);
  free(g);
  gras_exit();
  return 0;
}

/* **********************************************************************
 * Maestro code
 * **********************************************************************/

/* Function prototypes */
int maestro (int argc,char *argv[]);

/* XP setups */
const int buf_size = 0;
const int exp_size = 1024;// * 1024;
const int msg_size = 1024;
const int sat_size = 1024 * 1024 * 10 * 10;

static double XP(const char *bw1, const char *bw2,
		 const char *sat1, const char *sat2) {

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

static void kill_buddy(char *name,int port){
  gras_socket_t sock=gras_socket_client(name,port);
  gras_msg_send(sock,gras_msgtype_by_name("kill"),NULL);
  gras_socket_close(sock);
}
static void kill_buddy_dynar(void *b) {
  xbt_host_t buddy=*(xbt_host_t*)b;
  kill_buddy(buddy->name,buddy->port);
}

static void free_host(void *d){
  xbt_host_t h=*(xbt_host_t*)d;
  free(h->name);
  free(h);
}

static void simple_saturation(int argc, char*argv[]) {
  xbt_ex_t e;

  kill_buddy(argv[5],atoi(argv[6]));
  kill_buddy(argv[7],atoi(argv[8]));

  amok_bw_saturate_start(argv[1],atoi(argv[2]),argv[3],atoi(argv[4]),
			 sat_size*5,5);
  gras_os_sleep(3);
  TRY {
    amok_bw_saturate_stop(argv[1],atoi(argv[2]),NULL,NULL);
  } CATCH(e) {
    xbt_ex_free(e);
  }

 
  kill_buddy(argv[1],atoi(argv[2]));
  kill_buddy(argv[3],atoi(argv[4]));
}

static void full_fledged_saturation(int argc, char*argv[]) {
  xbt_ex_t e;

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
			       sat_size,120);
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

	  INFO4("TEST %s %s // %s %s",
		h1->name,h2->name,h3->name,h4->name);
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

  free(bw_sat);
  free(bw);
  xbt_dynar_map(hosts,kill_buddy_dynar);
  xbt_dynar_free(&hosts);
}


int maestro(int argc,char *argv[]) {

  gras_init(&argc,argv);
  amok_bw_init();


  //  simple_saturation(argc,argv);
  full_fledged_saturation(argc, argv);  

  gras_exit();
  return 0;

}
