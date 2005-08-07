/* $Id$ */

/* amok_bandwidth - Bandwidth tests facilities                              */

/* Copyright (c) 2003-5 Martin Quinson. All rights reserved.                */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "amok/Bandwidth/bandwidth_private.h"
#include "gras/messages.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(bw,amok,"Bandwidth testing");

static short _amok_bw_initialized = 0;

/** @brief module initialization; all participating nodes must run this */
void amok_bw_init(void) {
  gras_datadesc_type_t bw_request_desc, bw_res_desc, sat_request_desc;

  amok_base_init();

  if (! _amok_bw_initialized) {
	
     /* Build the datatype descriptions */ 
     bw_request_desc = gras_datadesc_struct("s_bw_request_t");
     gras_datadesc_struct_append(bw_request_desc,"host",
				 gras_datadesc_by_name("xbt_host_t"));
     gras_datadesc_struct_append(bw_request_desc,"buf_size",
				 gras_datadesc_by_name("unsigned long int"));
     gras_datadesc_struct_append(bw_request_desc,"exp_size",
				 gras_datadesc_by_name("unsigned long int"));
     gras_datadesc_struct_append(bw_request_desc,"msg_size",
				 gras_datadesc_by_name("unsigned long int"));
     gras_datadesc_struct_close(bw_request_desc);
     bw_request_desc = gras_datadesc_ref("bw_request_t",bw_request_desc);

     bw_res_desc = gras_datadesc_struct("s_bw_res_t");
     gras_datadesc_struct_append(bw_res_desc,"err",gras_datadesc_by_name("s_amok_remoterr_t"));
     gras_datadesc_struct_append(bw_res_desc,"timestamp",gras_datadesc_by_name("unsigned int"));
     gras_datadesc_struct_append(bw_res_desc,"seconds",gras_datadesc_by_name("double"));
     gras_datadesc_struct_append(bw_res_desc,"bw",gras_datadesc_by_name("double"));
     gras_datadesc_struct_close(bw_res_desc);
     bw_res_desc = gras_datadesc_ref("bw_res_t",bw_res_desc);

     sat_request_desc = gras_datadesc_struct("s_sat_request_desc_t");
     gras_datadesc_struct_append(sat_request_desc,"host",gras_datadesc_by_name("xbt_host_t"));
     gras_datadesc_struct_append(sat_request_desc,"msg_size",gras_datadesc_by_name("unsigned int"));
     gras_datadesc_struct_append(sat_request_desc,"timeout",gras_datadesc_by_name("unsigned int"));
     gras_datadesc_struct_close(sat_request_desc);
     sat_request_desc = gras_datadesc_ref("sat_request_t",sat_request_desc);
     
     /* Register the bandwidth messages */
     gras_msgtype_declare("BW handshake",     bw_request_desc);
     gras_msgtype_declare("BW handshake ACK", bw_request_desc);
     gras_msgtype_declare("BW request",       bw_request_desc);
     gras_msgtype_declare("BW result",        bw_res_desc);
     
     /* Register the saturation messages */
     gras_msgtype_declare("SAT start",   sat_request_desc);
     gras_msgtype_declare("SAT started", gras_datadesc_by_name("amok_remoterr_t"));
     gras_msgtype_declare("SAT begin",   sat_request_desc);
     gras_msgtype_declare("SAT begun",   gras_datadesc_by_name("amok_remoterr_t"));
     gras_msgtype_declare("SAT end",     NULL);
     gras_msgtype_declare("SAT ended",   gras_datadesc_by_name("amok_remoterr_t"));
     gras_msgtype_declare("SAT stop",    NULL);
     gras_msgtype_declare("SAT stopped", gras_datadesc_by_name("amok_remoterr_t"));
  }
   
  /* Register the callbacks */
  gras_cb_register(gras_msgtype_by_name("BW request"),
		   &amok_bw_cb_bw_request);
  gras_cb_register(gras_msgtype_by_name("BW handshake"),
		   &amok_bw_cb_bw_handshake);

  gras_cb_register(gras_msgtype_by_name("SAT start"),
		   &amok_bw_cb_sat_start);
  gras_cb_register(gras_msgtype_by_name("SAT begin"),
		   &amok_bw_cb_sat_begin);
  
  _amok_bw_initialized =1;
}

/** @brief module finalization */
void amok_bw_exit(void) {
  if (! _amok_bw_initialized)
    return;
   
  gras_cb_unregister(gras_msgtype_by_name("BW request"),
		     &amok_bw_cb_bw_request);
  gras_cb_unregister(gras_msgtype_by_name("BW handshake"),
		     &amok_bw_cb_bw_handshake);

  gras_cb_unregister(gras_msgtype_by_name("SAT start"),
		     &amok_bw_cb_sat_start);
  gras_cb_unregister(gras_msgtype_by_name("SAT begin"),
		     &amok_bw_cb_sat_begin);

  _amok_bw_initialized = 0;
}

/* ***************************************************************************
 * Bandwidth tests
 * ***************************************************************************/

/**
 * \brief bandwidth measurement between localhost and \e peer
 * 
 * \arg peer: A (regular) socket at which the the host with which we should conduct the experiment can be contacted
 * \arg buf_size: Size of the socket buffer
 * \arg exp_size: Total size of data sent across the network
 * \arg msg_size: Size of each message sent. Ie, (\e expSize % \e msgSize) messages will be sent.
 * \arg sec: where the result (in seconds) should be stored.
 * \arg bw: observed Bandwidth (in kb/s) 
 *
 * Conduct a bandwidth test from the local process to the given peer.
 * This call is blocking until the end of the experiment.
 *
 * Results are reported in last args, and sizes are in kb.
 */
void amok_bw_test(gras_socket_t peer,
		  unsigned long int buf_size,
		  unsigned long int exp_size,
		  unsigned long int msg_size,
	  /*OUT*/ double *sec, double *bw) {

  /* Measurement sockets for the experiments */
  gras_socket_t measMasterIn=NULL,measIn,measOut;
  int port;
  bw_request_t request,request_ack;
  xbt_ex_t e;
  
  for (port = 5000; port < 10000 && measMasterIn == NULL; port++) {
    TRY {
      measMasterIn = gras_socket_server_ext(++port,buf_size,1);
    } CATCH(e) {
      measMasterIn = NULL;
      if (port < 10000) {
	xbt_ex_free(e);
      } else {
	RETHROW0("Error caught while opening a measurement socket: %s");
      }
    }
  }
  
  request=xbt_new0(s_bw_request_t,1);
  request->buf_size=buf_size*1024;
  request->exp_size=exp_size*1024;
  request->msg_size=msg_size*1024;
  request->host.name = NULL;
  request->host.port = gras_socket_my_port(measMasterIn);
  VERB5("Handshaking with %s:%d to connect it back on my %d (expsize=%ld kb= %ld b)", 
	gras_socket_peer_name(peer),gras_socket_peer_port(peer), request->host.port,
	buf_size,request->buf_size);

  TRY {
    gras_msg_send(peer,gras_msgtype_by_name("BW handshake"),&request);
  } CATCH(e) {
    RETHROW0("Error encountered while sending the BW request: %s");
  }
  measIn = gras_socket_meas_accept(measMasterIn);

  TRY {
    gras_msg_wait(60,gras_msgtype_by_name("BW handshake ACK"),NULL,&request_ack);
  } CATCH(e) {
    RETHROW0("Error encountered while waiting for the answer to BW request: %s");
  }
  
  /* FIXME: What if there is a remote error? */
   
  TRY {
    measOut=gras_socket_client_ext(gras_socket_peer_name(peer),
				   request_ack->host.port, 
				   request->buf_size,1);
  } CATCH(e) {
    RETHROW2("Error encountered while opening the measurement socket to %s:%d for BW test: %s",
	     gras_socket_peer_name(peer),request_ack->host.port);
  }
  DEBUG1("Got ACK; conduct the experiment (msg_size=%ld)",request->msg_size);

  *sec=gras_os_time();
  TRY {
    gras_socket_meas_send(measOut,120,request->exp_size,request->msg_size);
    gras_socket_meas_recv(measIn,120,1,1);
  } CATCH(e) {
    gras_socket_close(measOut);
    gras_socket_close(measMasterIn);
    gras_socket_close(measIn);
    RETHROW0("Unable to conduct the experiment: %s");
  }

  *sec = gras_os_time() - *sec;
  *bw = ((double)exp_size) / *sec;

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

   sizes are in byte (got converted from kb my expeditor)
*/
int amok_bw_cb_bw_handshake(gras_socket_t  expeditor,
			    void          *payload) {
  gras_socket_t measMasterIn=NULL,measIn,measOut;
  bw_request_t request=*(bw_request_t*)payload;
  bw_request_t answer;
  xbt_ex_t e;
  int port;
  
  VERB5("Handshaked to connect to %s:%d (sizes: buf=%lu exp=%lu msg=%lu)",
	gras_socket_peer_name(expeditor),request->host.port,
	request->buf_size,request->exp_size,request->msg_size);     

  /* Build our answer */
  answer = xbt_new0(s_bw_request_t,1);
  
  for (port = 6000; port < 10000 && measMasterIn == NULL; port++) {
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
    gras_msg_send(expeditor, gras_msgtype_by_name("BW handshake ACK"), &answer);
  } CATCH(e) { 
    gras_socket_close(measMasterIn);
    gras_socket_close(measOut);
    /* FIXME: tell error to remote */
    RETHROW0("Error encountered while sending the answer: %s");
  }

  TRY {
    measIn = gras_socket_meas_accept(measMasterIn);
    DEBUG4("BW handshake answered. buf_size=%lu exp_size=%lu msg_size=%lu port=%d",
	   answer->buf_size,answer->exp_size,answer->msg_size,answer->host.port);

    gras_socket_meas_recv(measIn, 120,request->exp_size,request->msg_size);
    gras_socket_meas_send(measOut,120,1,1);
  } CATCH(e) {
    gras_socket_close(measMasterIn);
    gras_socket_close(measIn);
    gras_socket_close(measOut);
    /* FIXME: tell error to remote ? */
    RETHROW0("Error encountered while receiving the experiment: %s");
  }

  if (measIn != measMasterIn)
    gras_socket_close(measMasterIn);
  gras_socket_close(measIn);
  gras_socket_close(measOut);
  free(answer);
  free(request);
  DEBUG0("BW experiment done.");
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
 * \arg buf_size: Size of the socket buffer
 * \arg exp_size: Total size of data sent across the network
 * \arg msg_size: Size of each message sent. (\e expSize % \e msgSize) messages will be sent.
 * \arg sec: where the result (in seconds) should be stored.
 * \arg bw: observed Bandwidth (in kb/s)
 *
 * Conduct a bandwidth test from the process from_host:from_port to to_host:to_port.
 * This call is blocking until the end of the experiment.
 *
 * Results are reported in last args, and sizes are in kb.
 */
void amok_bw_request(const char* from_name,unsigned int from_port,
		     const char* to_name,unsigned int to_port,
		     unsigned long int buf_size,
		     unsigned long int exp_size,
		     unsigned long int msg_size,
	     /*OUT*/ double *sec, double*bw) {
  
  gras_socket_t sock;
  /* The request */
  bw_request_t request;
  bw_res_t result;

  request=xbt_new0(s_bw_request_t,1);
  request->buf_size=buf_size;
  request->exp_size=exp_size;
  request->msg_size=msg_size;

  request->host.name = (char*)to_name;
  request->host.port = to_port;

  sock = gras_socket_client(from_name,from_port);
  gras_msg_send(sock,gras_msgtype_by_name("BW request"),&request);
  free(request);

  gras_msg_wait(240,gras_msgtype_by_name("BW result"),NULL, &result);
  
  *sec=result->sec;
  *bw =result->bw;

  VERB6("BW test between %s:%d and %s:%d took %f sec, achieving %f kb/s",
	from_name,from_port, to_name,to_port,
	*sec,*bw);

  gras_socket_close(sock);
  free(result);
}

int amok_bw_cb_bw_request(gras_socket_t    expeditor,
			  void            *payload) {
			  
  /* specification of the test to run, and our answer */
  bw_request_t request = *(bw_request_t*)payload;
  bw_res_t result = xbt_new0(s_bw_res,1);
  gras_socket_t peer;

  peer = gras_socket_client(request->host.name,request->host.port);
  amok_bw_test(peer,
	       request->buf_size,request->exp_size,request->msg_size,
	       &(result->sec),&(result->bw));

  gras_msg_send(expeditor,gras_msgtype_by_name("BW result"),&result);

  gras_os_sleep(1);
  gras_socket_close(peer);
  free(request);
  free(result);
  
  return 1;
}

int amok_bw_cb_sat_start(gras_socket_t     expeditor,
			 void             *payload) {
   CRITICAL0("amok_bw_cb_sat_start; not implemented");
   return 1;
} 
int amok_bw_cb_sat_begin(gras_socket_t     expeditor,
			 void             *payload) {
   CRITICAL0("amok_bw_cb_sat_begin: not implemented");
   return 1;
}
