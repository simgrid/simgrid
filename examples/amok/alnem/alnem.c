/* ALNeM itself                                                             */

/* Copyright (c) 2005, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#include <gras.h>

#include <tbx_graph.h> /* alvin's graph toolbox (+ reconstruction algorithm) */

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
  xbt_error_t errcode;
  sensor_data_t *g=gras_userdata_new(sensor_data_t);  

  if ((errcode=gras_sock_server_open(4000,4000,&(g->sock)))) { 
    fprintf(stderr,"Sensor: Error %s encountered while opening the server socket\n",xbt_error_name(errcode));
    return 1;
  }

  if (grasbw_register_messages()) {
    gras_sock_close(g->sock);
    return 1;
  }

  while (1) {
    if ((errcode=gras_msg_handle(3600.0)) && errcode != timeout_error) {
      fprintf(stderr,"Sensor: Error '%s' while handling message\n",
	      xbt_error_name(errcode));
    }
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

#define MAXHOSTS 100

#define INTERF(graph,table,a,u,b,v) INTERFERENCE(table,\
                                                TBX_Graph_nodeSearch(graph,a),\
                                                TBX_Graph_nodeSearch(graph,u),\
                                                TBX_Graph_nodeSearch(graph,b),\
                                                TBX_Graph_nodeSearch(graph,v))

int maestro(int argc,char *argv[]) {
  int bufSize=32 * 1024;
  int expSize= 1024 * 1024;
  int msgSize=expSize;
  int satSize=msgSize * 100;
  double dummy,beginSim;
  xbt_error_t errcode;
  maestro_data_t *g=gras_userdata_new(maestro_data_t);

  double bw[MAXHOSTS][MAXHOSTS];
  double bw_sat[MAXHOSTS][MAXHOSTS];

  int a,b,c,d,begin;

  TBX_Graph_t graph = TBX_Graph_newGraph("Essai",0,NULL); /* a dummy graph containing all hosts */
  TBX_FIFO_t host_fifo = TBX_FIFO_newFIFO();
  TBX_InterfTable_t interf = NULL; /* the measured interferences */
  TBX_Graph_t builded_graph = NULL; /* the graph builded from the interferences */

  /* basics setups */
  if (argc>MAXHOSTS) {
    fprintf(stderr,"You gave more than %d sensors for this experiment. Increase the MAX HOSTS constant in alnem code to be bigger than this number.\n",argc);
    return 1;
  }

  if ((errcode=gras_sock_server_open(4000,5000,&(g->sock)))) { 
    fprintf(stderr,"MAESTRO: Error %s encountered while opening the server socket\n",xbt_error_name(errcode));
    return 1;
  }

  if (grasbw_register_messages()) {
    gras_sock_close(g->sock);
    return 1;
  }

  for (a=1; a<argc; a++) {
    TBX_FIFO_insert(host_fifo,TBX_Graph_newNode(graph,argv[a], NULL));
  }
  TBX_Graph_fixNodeNumbering(graph);
  interf=TBX_Graph_newInterfTable(host_fifo);
  TBX_Graph_graphInterfTableInit(graph,interf);

  /* measure the bandwidths */
  begin=time(NULL);
  beginSim=gras_time();
  for (a=1; a<argc; a++) {
    for (b=1; b<argc; b++) {
      int test=0;
       
      if (a==b) continue;
      fprintf(stderr,"BW XP(%s %s)=",argv[a],argv[b]); 
      while ((errcode=grasbw_request(argv[a],4000,argv[b],4000,bufSize,expSize,msgSize,
				  &dummy,&(bw[a][b]))) && (errcode == timeout_error)) {
	 test++;
	 if (test==10) {
	    fprintf(stderr,"MAESTRO: 10 Timeouts; giving up\n");
	    return 1;
	 }
      }
      if (errcode) {
	fprintf(stderr,"MAESTRO: Error %s encountered while doing the test\n",xbt_error_name(errcode));
	return 1;
      }
      fprintf(stderr,"%f Mb/s in %f sec\n",bw[a][b],dummy);
    }
  }
  fprintf(stderr,"Did all BW tests in %ld sec (%.2f simulated sec)\n",
	  time(NULL)-begin,gras_time()-beginSim);
      
  /* saturation tests */
  for (a=1; a<argc; a++) {
    for (b=1; b<argc; b++) {
      for (c=1 ;c<argc; c++) {
	INTERF(graph,interf,argv[a],argv[c],argv[b],argv[c])= 1;
      }
    }
  }
	  
  for (a=1; a<argc; a++) {
    for (b=1; b<argc; b++) {
      if (a==b) continue;
      	
      if ((errcode=grasbw_saturate_start(argv[a],4000,argv[b],4000,satSize,360000000))) {
	fprintf(stderr,"MAESTRO: Error %s encountered while starting saturation\n",
		xbt_error_name(errcode));
	return -1;
      }
      gras_sleep(1,0);

      begin=time(NULL);
      beginSim=gras_time();
      for (c=1 ;c<argc; c++) {
	if (a==c || b==c) continue;

	for (d=1 ;d<argc; d++) {
	  if (a==d || b==d || c==d) continue;
	  
	  if ((errcode=grasbw_request(argv[c],4000,argv[d],4000,bufSize,expSize,msgSize,
				      &dummy,&(bw_sat[c][d])))) {
	    fprintf(stderr,"MAESTRO: Error %s encountered in test\n",xbt_error_name(errcode));
	    return 1;
	  }
	  fprintf(stderr, "MAESTRO[%.2f sec]: SATURATED BW XP(%s %s // %s %s) => %f (%f vs %f)%s\n",
		  gras_time(),
		  argv[c],argv[d],argv[a],argv[b],
		  bw_sat[c][d]/bw[c][d],bw[c][d],bw_sat[c][d],

		  (bw_sat[c][d]/bw[c][d] < 0.75) ? " THERE IS SOME INTERFERENCE !!!":"");
	  INTERF(graph,interf,argv[c],argv[d],argv[a],argv[b])= 
	    (bw_sat[c][d]/bw[c][d] < 0.75) ? 1 : 0;
	}
      }

      if ((errcode=grasbw_saturate_stop(argv[a],4000,argv[b],4000))) {
	fprintf(stderr,"MAESTRO: Error %s encountered while stopping saturation\n",
		xbt_error_name(errcode));
	return -1;
      }
      fprintf(stderr,"Did an iteration on saturation pair in %ld sec (%.2f simulated sec)\n",
	      time(NULL)-begin, gras_time()-beginSim);
    }
  }

  /* reconstruct the graph */
  TBX_Graph_interferenceTableDump(interf);
  TBX_Graph_interferenceTableSave(interf,"interference.dat");
  begin=time(NULL);
  fprintf(stderr, "MAESTRO: Reconstruct the graph... ");
  builded_graph = TBX_Graph_exploreInterference(interf);
  TBX_Graph_exportToGraphViz(builded_graph, "toto.dot");
  fprintf(stderr, "done (took %d sec)",(int)time(NULL));

  /* end */
  TBX_Graph_freeGraph(graph,NULL,NULL,NULL);
  TBX_Graph_freeGraph(builded_graph,NULL,NULL,NULL);
  TBX_Graph_freeInterfTable(interf);

  gras_sleep(5,0);
  exit(0); /* FIXME: There is a bug in MSG preventing me from terminating this server properly */
  return gras_sock_close(g->sock);
}
