/* $Id$ */

/* saturate - link saturation demo of GRAS features                         */
/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#include <gras.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(saturate,"Messages specific to this example");

/* **********************************************************************
 * Sensor code
 * **********************************************************************/

/* Global private data */
typedef struct {
  gras_socket_t *sock;
} sensor_data_t;

/* Function prototypes */
int sensor (int argc,char *argv[]);

int sensor (int argc,char *argv[]) {
  xbt_error_t errcode;
  sensor_data_t *g=gras_userdata_new(sensor_data_t);  

  if ((errcode=gras_socket_server(4000,&(g->sock)))) {
    CRITICAL1("Sensor: Error %s encountered while opening the server socket",xbt_error_name(errcode));
    return 1;
  }

  if (grasbw_register_messages()) {
    gras_socket_close(g->sock);
    return 1;
  }

  while (1) {
    if ((errcode=gras_msg_handle(60.0)) && errcode != timeout_error) { 
       CRITICAL1("Sensor: Error '%s' while handling message",
	      xbt_error_name(errcode));
    }
  }

  gras_os_sleep(5,0);
  gras_socket_close(g->sock);
   
  return 0;
}

/* **********************************************************************
 * Maestro code
 * **********************************************************************/

/* Global private data */
typedef struct {
  gras_socket_t *sock;
} maestro_data_t;

/* Function prototypes */
int maestro (int argc,char *argv[]);
double XP(const char *bw1, const char *bw2, const char *sat1, const char *sat2);

double XP(const char *bw1, const char *bw2, const char *sat1, const char *sat2) {
  xbt_error_t errcode;
  int bufSize=32 * 1024;
  int expSize=64 * 1024;
  int msgSize=64 * 1024;
  int satSize=msgSize * 10;
  double sec, bw, sec_sat,bw_sat;

  if ((errcode=grasbw_request(bw1,4000,bw2,4000,bufSize,expSize,msgSize,&sec,&bw))) {
    fprintf(stderr,"MAESTRO: Error %s encountered while doing the test\n",xbt_error_name(errcode));
    return -1;
  }
   
  fprintf(stderr,"MAESTRO: BW(%s,%s) => %f sec, achieving %f Mb/s\n",bw1,bw2,sec,bw);

  if ((errcode=grasbw_saturate_start(sat1,4000,sat2,4000,satSize,60))) {
    fprintf(stderr,"MAESTRO: Error %s encountered while starting saturation\n",
	    xbt_error_name(errcode));
    return -1;
  }
  gras_os_sleep(1,0);
  if ((errcode=grasbw_request(bw1,4000,bw2,4000,bufSize,expSize,msgSize,&sec_sat,&bw_sat))) {
    fprintf(stderr,"MAESTRO: Error %s encountered while doing the test\n",xbt_error_name(errcode));
    return -1;
  }
   
  fprintf(stderr,"MAESTRO: BW(%s,%s//%s,%s) => %f sec, achieving %f Mb/s\n",
	 bw1,bw2,sat1,sat2,sec_sat,bw_sat);

  if ((errcode=grasbw_saturate_stop(sat1,4000,sat2,4000))) {
    fprintf(stderr,"MAESTRO: Error %s encountered while stopping saturation\n",
	    xbt_error_name(errcode));
    return -1;
  }

  if (bw_sat/bw < 0.7) {
    fprintf(stderr,"MAESTRO: THERE IS SOME INTERFERENCE !!!\n");
  } 
  if (bw/bw_sat < 0.7) {
    fprintf(stderr,"MAESTRO: THERE IS SOME INTERFERENCE (and Im a cretin) !!!\n");
  } 
  return bw_sat/bw;

}

//#define MAXHOSTS 33
#define MAXHOSTS 4

int maestro(int argc,char *argv[]) {
  int bufSize=32 * 1024;
  int expSize= 1024 * 1024;
  int msgSize=expSize;
  int satSize=msgSize * 100;
  double dummy,beginSim;
  xbt_error_t errcode;
  maestro_data_t *g=gras_userdata_new(maestro_data_t);
  //  const char *hosts[MAXHOSTS] = { "61", "62", "63", "69", "70", "77", "81", "83", "85", "87", "88", "95", "98", "107", "109", "111", "112", "121", "124", "125", "131", "145", "150", "156", "157", "162", "165", "168", "169", "170", "175", "177", "178" };
  const char *hosts[MAXHOSTS] = { "A", "B", "C", "D" };

  double bw[MAXHOSTS][MAXHOSTS];
  double bw_sat[MAXHOSTS][MAXHOSTS];

  int a,b,c,d,begin;

  if ((errcode=gras_socket_server(4000,&(g->sock)))) { 
    fprintf(stderr,"MAESTRO: Error %s encountered while opening the server socket\n",xbt_error_name(errcode));
    return 1;
  }

  if (grasbw_register_messages()) {
    gras_socket_close(g->sock);
    return 1;
  }

  begin=time(NULL);
  beginSim=gras_os_time();
  for (a=0; a<MAXHOSTS; a++) {
    for (b=0; b<MAXHOSTS; b++) {
      if (a==b) continue;
      fprintf(stderr,"BW XP(%s %s)=",hosts[a],hosts[b]); 
      if ((errcode=grasbw_request(hosts[a],4000,hosts[b],4000,bufSize,expSize,msgSize,
				  &dummy,&(bw[a][b])))) {
	fprintf(stderr,"MAESTRO: Error %s encountered while doing the test\n",xbt_error_name(errcode));
	return 1;
      }
      fprintf(stderr,"%f Mb/s in %f sec\n",bw[a][b],dummy);
    }
  }
  fprintf(stderr,"Did all BW tests in %ld sec (%.2f simulated sec)\n",
	  time(NULL)-begin,gras_os_time()-beginSim);
      
  for (a=0; a<MAXHOSTS; a++) {
    for (b=0; b<MAXHOSTS; b++) {
      if (a==b) continue;
      	
      if ((errcode=grasbw_saturate_start(hosts[a],4000,hosts[b],4000,satSize,360000000))) {
	fprintf(stderr,"MAESTRO: Error %s encountered while starting saturation\n",
		xbt_error_name(errcode));
	return -1;
      }
      gras_os_sleep(1,0);

      begin=time(NULL);
      beginSim=gras_os_time();
      for (c=0 ;c<MAXHOSTS; c++) {
	if (a==c) continue;
	if (b==c) continue;

	for (d=0 ;d<MAXHOSTS; d++) {
	  if (a==d) continue;
	  if (b==d) continue;
	  if (c==d) continue;
	  
	  if ((errcode=grasbw_request(hosts[c],4000,hosts[d],4000,bufSize,expSize,msgSize,
				      &dummy,&(bw_sat[c][d])))) {
	    fprintf(stderr,"MAESTRO: Error %s encountered in test\n",xbt_error_name(errcode));
	    return 1;
	  }
	  fprintf(stderr, "MAESTRO[%.2f sec]: SATURATED BW XP(%s %s // %s %s) => %f (%f vs %f)%s\n",
		  gras_os_time(),
		  hosts[c],hosts[d],hosts[a],hosts[b],
		  bw_sat[c][d]/bw[c][d],bw[c][d],bw_sat[c][d],

		  (bw_sat[c][d]/bw[c][d] < 0.7) ? " THERE IS SOME INTERFERENCE !!!":
		  ((bw[c][d]/bw_sat[c][d] < 0.7) ? " THERE IS SOME INTERFERENCE (and Im a cretin) !!!":
		   ""));
	}
      }

      if ((errcode=grasbw_saturate_stop(hosts[a],4000,hosts[b],4000))) {
	fprintf(stderr,"MAESTRO: Error %s encountered while stopping saturation\n",
		xbt_error_name(errcode));
	return -1;
      }
      fprintf(stderr,"Did an iteration on saturation pair in %ld sec (%.2f simulated sec)\n",
	      time(NULL)-begin, gras_os_time()-beginSim);
    }
  }

  gras_os_sleep(5,0);
  exit(0);
#if 0
  return 0;
  /* start saturation */
  fprintf(stderr,"MAESTRO: Start saturation with size %d\n",msgSize);
  if ((errcode=grasbw_saturate_start(argv[5],atoi(argv[6]),argv[7],atoi(argv[8]),msgSize*10,60))) {
    fprintf(stderr,"MAESTRO: Error %s encountered while starting saturation\n",
	    xbt_error_name(errcode));
    return 1;
  }
  fprintf(stderr,"MAESTRO: Saturation started\n");
  gras_os_sleep(5,0);

  /* test with saturation */
  if ((errcode=grasbw_request(argv[1],atoi(argv[2]),argv[3],atoi(argv[4]),
			      bufSize,expSize,msgSize,&sec,&bw))) {
    fprintf(stderr,"MAESTRO: Error %s encountered while doing the test\n",xbt_error_name(errcode));
    return 1;
  }
   
  fprintf(stderr,"MAESTRO: Experience3 (%d ko in msgs of %d ko with saturation) took %f sec, achieving %f Mb/s\n",
	  expSize/1024,msgSize/1024,
	  sec,bw);

  /* stop saturation */
  if ((errcode=grasbw_saturate_stop(argv[5],atoi(argv[6]),argv[7],atoi(argv[8])))) {
    fprintf(stderr,"MAESTRO: Error %s encountered while stopping saturation\n",
	    xbt_error_name(errcode));
    return 1;
  }

  /* test without saturation */
  if ((errcode=grasbw_request(argv[1],atoi(argv[2]),argv[3],atoi(argv[4]),
			      bufSize,expSize,msgSize,&sec,&bw))) {
    fprintf(stderr,"MAESTRO: Error %s encountered while doing the test\n",xbt_error_name(errcode));
    return 1;
  }
   
  fprintf(stderr,"MAESTRO: Experience4 (%d ko in msgs of %d ko, without saturation) took %f sec, achieving %f Mb/s\n",
	  expSize/1024,msgSize/1024,
	  sec,bw);

  gras_os_sleep(5,0);
#endif
  gras_socket_close(g->sock);
   
  return 0;
}
