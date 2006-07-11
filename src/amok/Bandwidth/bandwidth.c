/* $Id$ */

/* amok_bandwidth - Bandwidth tests facilities                              */

/* Copyright (c) 2003-6 Martin Quinson.                                     */
/* Copyright (c) 2006   Ahmed Harbaoui.                                     */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "amok/Bandwidth/bandwidth_private.h"
#include "gras/messages.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(amok_bw,amok,"Bandwidth testing");


/******************************
 * Stuff global to the module *
 ******************************/

static short _amok_bw_initialized = 0;

/** @brief module initialization; all participating nodes must run this */
void amok_bw_init(void) {

  amok_base_init();

  if (! _amok_bw_initialized) {
    amok_bw_bw_init();
    amok_bw_sat_init();
  }
   
  amok_bw_bw_join();
  amok_bw_sat_join();

  _amok_bw_initialized++;
}

/** @brief module finalization */
void amok_bw_exit(void) {
  if (! _amok_bw_initialized)
    return;
   
  amok_bw_bw_leave();
  amok_bw_sat_leave();

  _amok_bw_initialized--;
}

/* ***************************************************************************
 * Bandwidth tests
 * ***************************************************************************/
static int amok_bw_cb_bw_handshake(gras_msg_cb_ctx_t ctx, void *payload);
static int amok_bw_cb_bw_request(gras_msg_cb_ctx_t ctx, void *payload);

void amok_bw_bw_init() {
  gras_datadesc_type_t bw_request_desc, bw_res_desc;

  /* Build the Bandwidth datatype descriptions */ 
  bw_request_desc = gras_datadesc_struct("s_bw_request_t");
  gras_datadesc_struct_append(bw_request_desc,"host",
			      gras_datadesc_by_name("s_xbt_host_t"));
  gras_datadesc_struct_append(bw_request_desc,"buf_size",
			      gras_datadesc_by_name("unsigned long int"));
  gras_datadesc_struct_append(bw_request_desc,"exp_size",
			      gras_datadesc_by_name("unsigned long int"));
  gras_datadesc_struct_append(bw_request_desc,"msg_size",
			      gras_datadesc_by_name("unsigned long int"));
  gras_datadesc_struct_append(bw_request_desc,"min_duration",
			      gras_datadesc_by_name("double"));
  gras_datadesc_struct_close(bw_request_desc);
  bw_request_desc = gras_datadesc_ref("bw_request_t",bw_request_desc);
  
  bw_res_desc = gras_datadesc_struct("s_bw_res_t");
  gras_datadesc_struct_append(bw_res_desc,"timestamp",gras_datadesc_by_name("unsigned int"));
  gras_datadesc_struct_append(bw_res_desc,"seconds",gras_datadesc_by_name("double"));
  gras_datadesc_struct_append(bw_res_desc,"bw",gras_datadesc_by_name("double"));
  gras_datadesc_struct_close(bw_res_desc);
  bw_res_desc = gras_datadesc_ref("bw_res_t",bw_res_desc);
  
  gras_msgtype_declare_rpc("BW handshake",bw_request_desc,bw_request_desc);

  gras_msgtype_declare_rpc("BW reask",bw_request_desc,NULL);
  gras_msgtype_declare("BW stop", NULL);

  gras_msgtype_declare_rpc("BW request",  bw_request_desc,bw_res_desc);
}
void amok_bw_bw_join() {
  gras_cb_register(gras_msgtype_by_name("BW request"),
		   &amok_bw_cb_bw_request);
  gras_cb_register(gras_msgtype_by_name("BW handshake"),
		   &amok_bw_cb_bw_handshake);
}
void amok_bw_bw_leave() {
  gras_cb_unregister(gras_msgtype_by_name("BW request"),
		     &amok_bw_cb_bw_request);
  gras_cb_unregister(gras_msgtype_by_name("BW handshake"),
		     &amok_bw_cb_bw_handshake);
}

/**
 * \brief bandwidth measurement between localhost and \e peer
 * 
 * \arg peer: A (regular) socket at which the the host with which we should conduct the experiment can be contacted
 * \arg buf_size: Size of the socket buffer. If 0, a sain default is used (32k, but may change)
 * \arg exp_size: Total size of data sent across the network
 * \arg msg_size: Size of each message sent. Ie, (\e expSize % \e msgSize) messages will be sent.
 * \arg min_duration: The minimum wanted duration. When the test message is too little, you tend to measure the latency. This argument allows you to force the test to take at least, say one second.
 * \arg sec: where the result (in seconds) should be stored. If the experiment was done several times because the first one was too short, this is the timing of the last run only.
 * \arg bw: observed Bandwidth (in byte/s) 
 *
 * Conduct a bandwidth test from the local process to the given peer.
 * This call is blocking until the end of the experiment.
 *
 * Results are reported in last args, and sizes are in byte.
 */
void amok_bw_test(gras_socket_t peer,
		  unsigned long int buf_size,
		  unsigned long int exp_size,
		  unsigned long int msg_size,
		  double min_duration,
	  /*OUT*/ double *sec, double *bw) {

  /* Measurement sockets for the experiments */
  gras_socket_t measMasterIn=NULL,measIn,measOut=NULL;
  int port;
  bw_request_t request,request_ack;
  xbt_ex_t e;
  
  for (port = 5000; port < 10000 && measMasterIn == NULL; port++) {
    TRY {
      measMasterIn = gras_socket_server_ext(++port,buf_size,1);
    } CATCH(e) {
      measMasterIn = NULL;
      if (port == 10000 -1) {
	RETHROW0("Error caught while opening a measurement socket: %s");
      } else {
	xbt_ex_free(e);	
      }
    }
  }
  
  request=xbt_new0(s_bw_request_t,1);
  request->buf_size=buf_size;
  request->exp_size=exp_size;
  request->msg_size=msg_size;
  request->host.name = NULL;
  request->host.port = gras_socket_my_port(measMasterIn);
  DEBUG5("Handshaking with %s:%d to connect it back on my %d (expsize=%ld byte= %ld b)", 
	gras_socket_peer_name(peer),gras_socket_peer_port(peer), request->host.port,
	buf_size,request->buf_size);

  TRY {
    gras_msg_rpccall(peer,15,
		     gras_msgtype_by_name("BW handshake"),&request, &request_ack);
  } CATCH(e) {
    RETHROW0("Error encountered while sending the BW request: %s");
  }
  measIn = gras_socket_meas_accept(measMasterIn);
   
  TRY {
    measOut=gras_socket_client_ext(gras_socket_peer_name(peer),
				   request_ack->host.port, 
				   request->buf_size,1);
  } CATCH(e) {
    RETHROW2("Error encountered while opening the measurement socket to %s:%d for BW test: %s",
	     gras_socket_peer_name(peer),request_ack->host.port);
  }
  DEBUG1("Got ACK; conduct the experiment (msg_size=%ld)",request->msg_size);

  *sec = 0;
  do {
    if (*sec>0) {
      double meas_duration=*sec;
      request->exp_size = request->exp_size * (min_duration / meas_duration) * 1.1;
      request->msg_size = request->msg_size * (min_duration / meas_duration) * 1.1;


      DEBUG5("The experiment was too short (%f sec<%f sec). Redo it with exp_size=%ld msg_size=%ld (got %fkb/s)",
	     meas_duration,min_duration,request->exp_size,request->msg_size,((double)exp_size) / *sec/1024);
      gras_msg_rpccall(peer, 60, gras_msgtype_by_name("BW reask"),&request, NULL);      
      DEBUG0("Peer is ready for another round of fun");
    }

    *sec=gras_os_time();
    TRY {
      gras_socket_meas_send(measOut,120,request->exp_size,request->msg_size);
      DEBUG0("Data sent. Wait ACK");
      gras_socket_meas_recv(measIn,120,1,1);
    } CATCH(e) {
      gras_socket_close(measOut);
      gras_socket_close(measMasterIn);
      gras_socket_close(measIn);
      RETHROW0("Unable to conduct the experiment: %s");
    }
    DEBUG0("Experiment done");

    *sec = gras_os_time() - *sec;
    *bw = ((double)request->exp_size) / *sec;
  } while (*sec < min_duration);

  DEBUG2("This measurement was long enough (%f sec; found %f b/s). Stop peer",
	 *sec,*bw);
  gras_msg_send(peer, gras_msgtype_by_name("BW stop"), NULL);      

  free(request_ack);
  free(request);
  if (measIn != measMasterIn)
    gras_socket_close(measIn);
  gras_socket_close(measMasterIn);
  gras_socket_close(measOut);
}


/* Callback to the "BW handshake" message: 
   opens a server measurement socket,
   indicate its port in an "BW handshaked" message,
   receive the corresponding data on the measurement socket, 
   close the measurment socket

   sizes are in byte
*/
int amok_bw_cb_bw_handshake(gras_msg_cb_ctx_t  ctx,
			    void          *payload) {
  gras_socket_t expeditor = gras_msg_cb_ctx_from(ctx);
  gras_socket_t measMasterIn=NULL,measIn=NULL,measOut=NULL;
  bw_request_t request=*(bw_request_t*)payload;
  bw_request_t answer;
  xbt_ex_t e;
  int port;
  int tooshort = 1;
  gras_msg_cb_ctx_t ctx_reask;
  static xbt_dynar_t msgtwaited=NULL;
  
  DEBUG5("Handshaked to connect to %s:%d (sizes: buf=%lu exp=%lu msg=%lu)",
	gras_socket_peer_name(expeditor),request->host.port,
	request->buf_size,request->exp_size,request->msg_size);     

  /* Build our answer */
  answer = xbt_new0(s_bw_request_t,1);
  
  for (port = 6000; port <= 10000 && measMasterIn == NULL; port++) {
    TRY {
      measMasterIn = gras_socket_server_ext(port,request->buf_size,1);
    } CATCH(e) {
      measMasterIn = NULL;
      if (port < 10000)
	xbt_ex_free(e);
      else
	/* FIXME: tell error to remote */
	RETHROW0("Error encountered while opening a measurement server socket: %s");
    }
  }
   
  answer->buf_size=request->buf_size;
  answer->exp_size=request->exp_size;
  answer->msg_size=request->msg_size;
  answer->host.port=gras_socket_my_port(measMasterIn);

  TRY {
    gras_msg_rpcreturn(60,ctx,&answer);
  } CATCH(e) { 
    gras_socket_close(measMasterIn);
    /* FIXME: tell error to remote */
    RETHROW0("Error encountered while sending the answer: %s");
  }


  /* Don't connect asap to leave time to other side to enter the accept() */
  TRY {
    measOut = gras_socket_client_ext(gras_socket_peer_name(expeditor),
				     request->host.port,
				     request->buf_size,1);
  } CATCH(e) {
    RETHROW2("Error encountered while opening a measurement socket back to %s:%d : %s", 
	     gras_socket_peer_name(expeditor),request->host.port);
    /* FIXME: tell error to remote */
  }

  TRY {
    measIn = gras_socket_meas_accept(measMasterIn);
    DEBUG4("BW handshake answered. buf_size=%lu exp_size=%lu msg_size=%lu port=%d",
	   answer->buf_size,answer->exp_size,answer->msg_size,answer->host.port);
  } CATCH(e) {
    gras_socket_close(measMasterIn);
    gras_socket_close(measIn);
    gras_socket_close(measOut);
    /* FIXME: tell error to remote ? */
    RETHROW0("Error encountered while opening the meas socket: %s");
  }

  if (!msgtwaited) {
    msgtwaited = xbt_dynar_new(sizeof(gras_msgtype_t),NULL);
    xbt_dynar_push(msgtwaited,gras_msgtype_by_name("BW stop"));
    xbt_dynar_push(msgtwaited,gras_msgtype_by_name("BW reask"));
  }

  while (tooshort) {
    void *payload;
    int msggot;
    TRY {
      DEBUG0("Recv / Send the experiment");
      gras_socket_meas_recv(measIn, 120,request->exp_size,request->msg_size);
      gras_socket_meas_send(measOut,120,1,1);
      DEBUG0("ACK sent");
    } CATCH(e) {
      gras_socket_close(measMasterIn);
      gras_socket_close(measIn);
      gras_socket_close(measOut);
      /* FIXME: tell error to remote ? */
      RETHROW0("Error encountered while receiving the experiment: %s");
    }
    gras_msg_wait_or(60,msgtwaited,&ctx_reask,&msggot,&payload);
    switch(msggot) {
    case 0: /* BW stop */
      tooshort = 0;
      break;
    case 1: /* BW reask */
      tooshort = 1;
      free(request);
      request = (bw_request_t)payload;
      VERB0("Return the reasking RPC");
      gras_msg_rpcreturn(60,ctx_reask,NULL);
    }
    gras_msg_cb_ctx_free(ctx_reask);
  }

  if (measIn != measMasterIn)
    gras_socket_close(measMasterIn);
  gras_socket_close(measIn);
  gras_socket_close(measOut);
  free(answer);
  free(request);
  VERB0("BW experiment done.");
  return 1;
}

/**
 * \brief request a bandwidth measurement between two remote hosts
 *
 * \arg from_name: Name of the first host 
 * \arg from_port: port on which the first process is listening for messages
 * \arg to_name: Name of the second host 
 * \arg to_port: port on which the second process is listening (for messages, do not 
 * give a measurement socket here. The needed measurement sockets will be created 
 * automatically and negociated between the peers)
 * \arg buf_size: Size of the socket buffer. If 0, a sain default is used (32k, but may change)
 * \arg exp_size: Total size of data sent across the network
 * \arg msg_size: Size of each message sent. (\e expSize % \e msgSize) messages will be sent.
 * \arg sec: where the result (in seconds) should be stored.
 * \arg bw: observed Bandwidth (in byte/s)
 *
 * Conduct a bandwidth test from the process from_host:from_port to to_host:to_port.
 * This call is blocking until the end of the experiment.
 *
 * Results are reported in last args, and sizes are in bytes.
 */
void amok_bw_request(const char* from_name,unsigned int from_port,
		     const char* to_name,unsigned int to_port,
		     unsigned long int buf_size,
		     unsigned long int exp_size,
		     unsigned long int msg_size,
		     double min_duration,
	     /*OUT*/ double *sec, double*bw) {
  
  gras_socket_t sock;
  /* The request */
  bw_request_t request;
  bw_res_t result;

  request=xbt_new0(s_bw_request_t,1);
  request->buf_size=buf_size;
  request->exp_size=exp_size;
  request->msg_size=msg_size;
  request->min_duration = min_duration;

  request->host.name = (char*)to_name;
  request->host.port = to_port;

  sock = gras_socket_client(from_name,from_port);
  DEBUG4("Ask for a BW test between %s:%d and %s:%d",	from_name,from_port, to_name,to_port);

  gras_msg_rpccall(sock,20*60,gras_msgtype_by_name("BW request"), &request, &result);
  
  if (sec)
    *sec=result->sec;
  if (bw)
    *bw =result->bw;

  VERB6("BW test (%s:%d -> %s:%d) took %f sec (%f kb/s)",
	from_name,from_port, to_name,to_port,
	result->sec,((double)result->bw)/1024.0);

  gras_socket_close(sock);
  free(result);
  free(request);
}

int amok_bw_cb_bw_request(gras_msg_cb_ctx_t ctx,
			  void            *payload) {
			  
  /* specification of the test to run, and our answer */
  bw_request_t request = *(bw_request_t*)payload;
  bw_res_t result = xbt_new0(s_bw_res_t,1);
  gras_socket_t peer,asker;

  asker=gras_msg_cb_ctx_from(ctx);
  VERB4("Asked by %s:%d to conduct a bw XP with %s:%d",	
	gras_socket_peer_name(asker),gras_socket_peer_port(asker),
	request->host.name,request->host.port);
  peer = gras_socket_client(request->host.name,request->host.port);
  amok_bw_test(peer,
	       request->buf_size,request->exp_size,request->msg_size,
	       request->min_duration,
	       &(result->sec),&(result->bw));

  gras_msg_rpcreturn(240,ctx,&result);

  gras_os_sleep(1);
  gras_socket_close(peer); /* FIXME: it should be blocking in RL until everything is sent */
  free(request->host.name);
  free(request);
  free(result);
  
  return 1;
}

/** \brief builds a matrix of results of bandwidth measurement */
double * amok_bw_matrix(xbt_dynar_t hosts,
			int buf_size_bw, int exp_size_bw, int msg_size_bw,
			double min_duration) { 
  double sec;
  /* construction of matrices for bandwith and latency */


  int i,j,len=xbt_dynar_length(hosts);

  double *matrix_res = xbt_new0(double, len*len);
  xbt_host_t h1,h2;

  xbt_dynar_foreach (hosts,i,h1) {
    xbt_dynar_foreach (hosts,j,h2) {
      if (i!=j) {
        /* Mesurements of Bandwidth */
        amok_bw_request(h1->name,h1->port,h2->name,h2->port,
                        buf_size_bw,exp_size_bw,msg_size_bw,min_duration,
			&sec,&matrix_res[i*len + j]);
      } 
    }
  }
  return matrix_res;
}
