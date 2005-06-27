/* $Id$ */

/* amok_bandwidth - Bandwidth tests facilities                              */

/* Copyright (c) 2003-5 Martin Quinson. All rights reserved.                */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

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
xbt_error_t amok_bw_test(gras_socket_t peer,
			 unsigned long int buf_size,
			 unsigned long int exp_size,
			 unsigned long int msg_size,
			 /*OUT*/ double *sec, double *bw) {

  /* Measurement sockets for the experiments */
  gras_socket_t measMasterIn,measIn,measOut;
  int port;
  xbt_error_t errcode;
  bw_request_t request,request_ack;
  
  for (port = 5000, errcode = system_error;
       errcode == system_error && port < 10000;
       errcode = gras_socket_server_ext(++port,buf_size,1,&measMasterIn));
  if (errcode != no_error) {
    ERROR1("Error %s encountered while opening a measurement socket",
	   xbt_error_name(errcode));
    return errcode;
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

  if ((errcode=gras_msg_send(peer,gras_msgtype_by_name("BW handshake"),&request))) {
    ERROR1("Error %s encountered while sending the BW request.", xbt_error_name(errcode));
    return errcode;
  }
  TRY(gras_socket_meas_accept(measMasterIn,&measIn));

  if ((errcode=gras_msg_wait(60,gras_msgtype_by_name("BW handshake ACK"),
			     NULL,&request_ack))) {
    ERROR1("Error %s encountered while waiting for the answer to BW request.\n",
	    xbt_error_name(errcode));
    return errcode;
  }
  
  /* FIXME: What if there is a remote error? */
   
  if((errcode=gras_socket_client_ext(gras_socket_peer_name(peer),
				     request_ack->host.port, 
				     request->buf_size,1,&measOut))) {
    ERROR3("Error %s encountered while opening the measurement socket to %s:%d for BW test\n",
	    xbt_error_name(errcode),gras_socket_peer_name(peer),request_ack->host.port);
    return errcode;
  }
  DEBUG1("Got ACK; conduct the experiment (msg_size=%ld)",request->msg_size);

  *sec=gras_os_time();
  TRY(gras_socket_meas_send(measOut,120,request->exp_size,request->msg_size));
  TRY(gras_socket_meas_recv(measIn,120,1,1));

  /*catch
    ERROR1("Error %s encountered while sending the BW experiment.",
	    xbt_error_name(errcode));
    gras_socket_close(measOut);
    gras_socket_close(measMasterIn);
    gras_socket_close(measIn);
  */

  *sec = gras_os_time() - *sec;
  *bw = ((double)exp_size) / *sec;

  free(request_ack);
  free(request);
  if (measIn != measMasterIn)
    gras_socket_close(measIn);
  gras_socket_close(measMasterIn);
  gras_socket_close(measOut);
  return errcode;
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
  gras_socket_t measMasterIn,measIn,measOut;
  bw_request_t request=*(bw_request_t*)payload;
  bw_request_t answer;
  xbt_error_t errcode;
  int port;
  
  VERB5("Handshaked to connect to %s:%d (sizes: buf=%lu exp=%lu msg=%lu)",
	gras_socket_peer_name(expeditor),request->host.port,
	request->buf_size,request->exp_size,request->msg_size);     

  /* Build our answer */
  answer = xbt_new0(s_bw_request_t,1);
  
  for (port = 6000, errcode = system_error;
       errcode == system_error;
       errcode = gras_socket_server_ext(++port,request->buf_size,1,&measMasterIn));
  if (errcode != no_error) {
    ERROR1("Error %s encountered while opening a measurement server socket", xbt_error_name(errcode));
    /* FIXME: tell error to remote */
    return 1;
  }
   
  answer->buf_size=request->buf_size;
  answer->exp_size=request->exp_size;
  answer->msg_size=request->msg_size;
  answer->host.port=gras_socket_my_port(measMasterIn);

  /* Don't connect asap to leave time to other side to enter the accept() */
  if ((errcode=gras_socket_client_ext(gras_socket_peer_name(expeditor),
				      request->host.port,
				      request->buf_size,1,&measOut))) { 
    ERROR3("Error '%s' encountered while opening a measurement socket back to %s:%d", 
	   xbt_error_name(errcode),gras_socket_peer_name(expeditor),request->host.port);
    /* FIXME: tell error to remote */
    return 1;
  }


  if ((errcode=gras_msg_send(expeditor,
			     gras_msgtype_by_name("BW handshake ACK"),
			     &answer))) {
    ERROR1("Error %s encountered while sending the answer.",
	    xbt_error_name(errcode));
    gras_socket_close(measMasterIn);
    gras_socket_close(measOut);
    /* FIXME: tell error to remote */
    return 1;
  }
  TRY(gras_socket_meas_accept(measMasterIn,&measIn));
  DEBUG4("BW handshake answered. buf_size=%lu exp_size=%lu msg_size=%lu port=%d",
	answer->buf_size,answer->exp_size,answer->msg_size,answer->host.port);

  TRY(gras_socket_meas_recv(measIn, 120,request->exp_size,request->msg_size));
  TRY(gras_socket_meas_send(measOut,120,1,1));

  /*catch
    ERROR1("Error %s encountered while receiving the experiment.",
	    xbt_error_name(errcode));
    gras_socket_close(measMasterIn);
    gras_socket_close(measIn);
    gras_socket_close(measOut);
    * FIXME: tell error to remote ? *
    return 1;
    }*/

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
xbt_error_t amok_bw_request(const char* from_name,unsigned int from_port,
			    const char* to_name,unsigned int to_port,
			    unsigned long int buf_size,
			    unsigned long int exp_size,
			    unsigned long int msg_size,
			    /*OUT*/ double *sec, double*bw) {
  
  gras_socket_t sock;
  xbt_error_t errcode;
  /* The request */
  bw_request_t request;
  bw_res_t result;

  request=xbt_new0(s_bw_request_t,1);
  request->buf_size=buf_size;
  request->exp_size=exp_size;
  request->msg_size=msg_size;

  request->host.name = (char*)to_name;
  request->host.port = to_port;

  TRY(gras_socket_client(from_name,from_port,&sock));
  TRY(gras_msg_send(sock,gras_msgtype_by_name("BW request"),&request));
  free(request);

  TRY(gras_msg_wait(240,gras_msgtype_by_name("BW result"),NULL, &result));
  
  *sec=result->sec;
  *bw =result->bw;

  VERB6("BW test between %s:%d and %s:%d took %f sec, achieving %f kb/s",
	from_name,from_port, to_name,to_port,
	*sec,*bw);

  gras_socket_close(sock);
  free(result);
  return no_error;
}

int amok_bw_cb_bw_request(gras_socket_t    expeditor,
			  void            *payload) {
			  
  xbt_error_t errcode;			  
  /* specification of the test to run, and our answer */
  bw_request_t request = *(bw_request_t*)payload;
  bw_res_t result = xbt_new0(s_bw_res,1);
  gras_socket_t peer;

  TRY(gras_socket_client(request->host.name,request->host.port,&peer));
  TRY(amok_bw_test(peer,
		   request->buf_size,request->exp_size,request->msg_size,
		   &(result->sec),&(result->bw)));

  TRY(gras_msg_send(expeditor,gras_msgtype_by_name("BW result"),&result));

  gras_os_sleep(1);
  gras_socket_close(peer);
  free(request);
  free(result);
  
  return 1;

#if 0
  char* to_name=gras_msg_ctn(msg,0,0,msgHost_t).host;
  unsigned int to_port=gras_msg_ctn(msg,0,0,msgHost_t).port;

  unsigned int bufSize=gras_msg_ctn(msg,1,0,BwExp_t).bufSize;
  unsigned int expSize=gras_msg_ctn(msg,1,0,BwExp_t).expSize;
  unsigned int msgSize=gras_msg_ctn(msg,1,0,BwExp_t).msgSize;
  /* our answer */
  msgError_t *error;
  msgResult_t *res;

  if ((error->errcode=grasbw_test(to_name,to_port,bufSize,expSize,msgSize,
				  &(res[0].value),&(res[1].value) ))) {
    fprintf(stderr,
	    "%s:%d:grasbw_cbRequest: Error %s encountered while doing the test\n",
	    __FILE__,__LINE__,xbt_error_name(error->errcode));
    strncpy(error->errmsg,"Error within grasbw_test",ERRMSG_LEN);
    gras_msg_new_and_send(msg->sock,GRASMSG_BW_RESULT,2,
		   error,1,
		   res,2);
    return 1;
  }
  res[0].timestamp = (unsigned int) gras_time();
  res[1].timestamp = (unsigned int) gras_time();
  gras_msg_new_and_send(msg->sock,GRASMSG_BW_RESULT,2,
		 error,1,
		 res,2);
  gras_msg_free(msg);
  return 1;
#endif
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
#if 0
/* ***************************************************************************
 * Link saturation
 * ***************************************************************************/

xbt_error_t grasbw_saturate_start(const char* from_name,unsigned int from_port,
				  const char* to_name,unsigned int to_port,
				  unsigned int msgSize, unsigned int timeout) {
  gras_sock_t *sock;
  xbt_error_t errcode;
  /* The request */
  SatExp_t *request;
  msgHost_t *target;
  /* answer */
  gras_msg_t *answer;

  if((errcode=gras_sock_client_open(from_name,from_port,&sock))) {
    fprintf(stderr,"%s:%d:saturate_start(): Error %s encountered while contacting peer\n",
	    __FILE__,__LINE__,xbt_error_name(errcode));
    return errcode;
  }
  if (!(request=(SatExp_t *)malloc(sizeof(SatExp_t))) ||
      !(target=(msgHost_t*)malloc(sizeof(msgHost_t)))) {
    fprintf(stderr,"%s:%d:saturate_start(): Malloc error\n",__FILE__,__LINE__);
    gras_sock_close(sock);
    return malloc_error;    
  }

  request->timeout=timeout;
  request->msgSize=msgSize;

  strcpy(target->host,to_name);
  target->port=to_port;

  if ((errcode=gras_msg_new_and_send(sock,GRASMSG_SAT_START, 2, 
			      target,1,
			      request,1))) {
    fprintf(stderr,"%s:%d:saturate_start(): Error %s encountered while sending the request.\n",
	    __FILE__,__LINE__,xbt_error_name(errcode));
    gras_sock_close(sock);
    return errcode;
  }
  if ((errcode=gras_msg_wait(120,GRASMSG_SAT_STARTED,&answer))) {
    fprintf(stderr,"%s:%d:saturate_start(): Error %s encountered while waiting for the ACK.\n",
	    __FILE__,__LINE__,xbt_error_name(errcode));
    gras_sock_close(sock);
    return errcode;
  }

  if((errcode=gras_msg_ctn(answer,0,0,msgError_t).errcode)) {
    fprintf(stderr,"%s:%d:saturate_start(): Peer reported error %s (%s).\n",
	    __FILE__,__LINE__,xbt_error_name(errcode),gras_msg_ctn(answer,0,0,msgError_t).errmsg);
    gras_msg_free(answer);
    gras_sock_close(sock);
    return errcode;
  }

  gras_msg_free(answer);
  gras_sock_close(sock);
  return no_error;
}

int grasbw_cbSatStart(gras_msg_t *msg) {
  gras_rawsock_t *raw;
  gras_sock_t *sock;
  xbt_error_t errcode;
  double start; /* time to timeout */

  /* specification of the test to run */
  char* to_name=gras_msg_ctn(msg,0,0,msgHost_t).host;
  unsigned int to_port=gras_msg_ctn(msg,0,0,msgHost_t).port;

  unsigned int msgSize=gras_msg_ctn(msg,1,0,SatExp_t).msgSize;
  unsigned int timeout=gras_msg_ctn(msg,1,0,SatExp_t).timeout;
  unsigned int raw_port;

  /* The request */
  SatExp_t *request;
  /* answer */
  gras_msg_t *answer;

  /*
  fprintf(stderr,"grasbw_cbSatStart(sd=%p)\n",msg->sock);
  fprintf(stderr,"(server=%d,raw=%d,fromPID=%d,toPID=%d,toHost=%p,toPort=%d,toChan=%d)\n",
	  msg->sock->server_sock,msg->sock->raw_sock,msg->sock->from_PID,
	  msg->sock->to_PID,msg->sock->to_host,msg->sock->to_port,msg->sock->to_chan);
  */

  /* Negociate the saturation with the peer */
  if((errcode=gras_sock_client_open(to_name,to_port,&sock))) {
    fprintf(stderr,"cbSatStart(): Error %s encountered while contacting peer\n",
	    xbt_error_name(errcode));
    grasRepportError(msg->sock,GRASMSG_SAT_STARTED,1,
		     "cbSatStart: Severe error: Cannot send error status to requester!!\n",
		     errcode,"Cannot contact peer.\n");
    return 1;
  }
  if (!(request=(SatExp_t *)malloc(sizeof(SatExp_t)))) {
    fprintf(stderr,"cbSatStart(): Malloc error\n");
    gras_sock_close(sock);
    grasRepportError(msg->sock,GRASMSG_SAT_STARTED,1,
		     "cbSatStart: Severe error: Cannot send error status to requester!!\n",
		     malloc_error,"Cannot build request.\n");
    return 1;    
  }

  request->timeout=gras_msg_ctn(msg,1,0,SatExp_t).timeout;
  request->msgSize=gras_msg_ctn(msg,1,0,SatExp_t).msgSize;

  if ((errcode=gras_msg_new_and_send(sock,GRASMSG_SAT_BEGIN, 1, 
			      request,1))) {
    fprintf(stderr,"cbSatStart(): Error %s encountered while sending the request.\n",
	    xbt_error_name(errcode));
    grasRepportError(msg->sock,GRASMSG_SAT_STARTED,1,
		     "cbSatStart: Severe error: Cannot send error status to requester!!\n",
		     errcode,"Cannot send request.\n");
    gras_sock_close(sock);
    return 1;
  }

  if ((errcode=gras_msg_wait(120,GRASMSG_SAT_BEGUN,&answer))) {
    fprintf(stderr,"cbSatStart(): Error %s encountered while waiting for the ACK.\n",
	    xbt_error_name(errcode));
    gras_sock_close(sock);

    grasRepportError(msg->sock,GRASMSG_SAT_STARTED,1,
		     "cbSatStart: Severe error: Cannot send error status to requester!!\n",
		     errcode,
		     "Cannot receive the ACK.\n");
    return 1;
  }

  if((errcode=gras_msg_ctn(answer,0,0,msgError_t).errcode)) {
    fprintf(stderr,"cbSatStart(): Peer reported error %s (%s).\n",
	    xbt_error_name(errcode),gras_msg_ctn(answer,0,0,msgError_t).errmsg);

    grasRepportError(msg->sock,GRASMSG_SAT_STARTED,1,
		     "cbSatStart: Severe error: Cannot send error status to requester!!\n",
		     errcode,
		     "Peer repported '%s'.\n",gras_msg_ctn(answer,0,0,msgError_t).errmsg);
    gras_msg_free(answer);
    gras_sock_close(sock);
    return 1;
  }

  raw_port=gras_msg_ctn(answer,1,0,SatExp_t).port;

  if ((errcode=gras_rawsock_client_open(to_name,raw_port,msgSize,&raw))) {
    fprintf(stderr,"cbSatStart(): Error %s while opening raw socket to %s:%d.\n",
	    xbt_error_name(errcode),to_name,gras_msg_ctn(answer,1,0,SatExp_t).port);

    grasRepportError(msg->sock,GRASMSG_SAT_STARTED,1,
		     "cbSatStart: Severe error: Cannot send error status to requester!!\n",
		     errcode,"Cannot open raw socket.\n");
    gras_sock_close(sock);
    return 1;
  }

  /* send a train of data before repporting that XP is started */
  if ((errcode=gras_rawsock_send(raw,msgSize,msgSize))) {
    fprintf(stderr,"cbSatStart: Failure %s during raw send\n",xbt_error_name(errcode));
    grasRepportError(msg->sock,GRASMSG_SAT_STARTED,1,
		     "cbSatStart: Severe error: Cannot send error status to requester!!\n",
		     errcode,"Cannot raw send.\n");
    gras_sock_close(sock);
    gras_rawsock_close(raw);
    return 1;
  }
  
  grasRepportError(msg->sock,GRASMSG_SAT_STARTED,1,
		   "cbSatStart: Severe error: Cannot send error status to requester!!\n",
		   no_error,"Saturation started");
  gras_msg_free(answer);
  gras_msg_free(msg);
  
  /* Do the saturation until we get a SAT_STOP message or until we timeout the whole XP*/
  start=gras_time();
  while (gras_msg_wait(0,GRASMSG_SAT_STOP,&msg)==timeout_error && 
	 gras_time()-start < timeout) {
    if ((errcode=gras_rawsock_send(raw,msgSize,msgSize))) {
      fprintf(stderr,"cbSatStart: Failure %s during raw send\n",xbt_error_name(errcode));
      /* our error message do not interess anyone. SAT_STOP will do nothing. */
      gras_sock_close(sock);
      gras_rawsock_close(raw);
      return 1;
    } 
  }
  if (gras_time()-start > timeout) {
    fprintf(stderr,"The saturation experiment did timeout. Stop it NOW\n");
    gras_sock_close(sock);
    gras_rawsock_close(raw);
    return 1;
  }

  /* Handle the SAT_STOP which broke the previous while */
  
  if ((errcode=gras_msg_new_and_send(sock, GRASMSG_SAT_END,0))) {
    fprintf(stderr,"cbSatStart(): Cannot tell peer to stop saturation\n");

    grasRepportError(msg->sock,GRASMSG_SAT_STOPPED,1,
		     "cbSatStart: Severe error: Cannot send error status to requester!!\n",
		     errcode,"Sending SAT_END to peer failed.\n");
    gras_sock_close(sock);
    gras_rawsock_close(raw);
    return 1;
  }
  
  if ((errcode=gras_msg_wait(60,GRASMSG_SAT_ENDED,&answer))) {
    fprintf(stderr,"cbSatStart(): Peer didn't ACK the end\n");

    grasRepportError(msg->sock,GRASMSG_SAT_STOPPED,1,
		     "cbSatStart: Severe error: Cannot send error status to requester!!\n",
		     errcode,"Receiving SAT_ENDED from peer failed.\n");
    gras_sock_close(sock);
    gras_rawsock_close(raw);
    return 1;
  }
  grasRepportError(msg->sock,GRASMSG_SAT_STOPPED,1,
		   "cbSatStart: Severe error: Cannot send error status to requester!!\n",
		   no_error,"");

  gras_sock_close(sock);
  gras_rawsock_close(raw);
  gras_msg_free(answer);
  gras_msg_free(msg);

  return 1;  
}

int grasbw_cbSatBegin(gras_msg_t *msg) {
  gras_rawsock_t *raw;
  xbt_error_t errcode;
  double start; /* timer */
  /* request */
  unsigned int msgSize=gras_msg_ctn(msg,0,0,SatExp_t).msgSize;
  unsigned int timeout=gras_msg_ctn(msg,0,0,SatExp_t).timeout;
  /* answer */
  SatExp_t *request;
  msgError_t *error;

  if (!(request=(SatExp_t*)malloc(sizeof(SatExp_t))) ||
      !(error=(msgError_t *)malloc(sizeof(msgError_t)))) {
    fprintf(stderr,"cbSatBegin(): Malloc error\n");
    grasRepportError(msg->sock,GRASMSG_SAT_BEGUN,2,
		     "cbSatBegin: Severe error: Cannot send error status to requester!!\n",
		     malloc_error,"Malloc error");
    return 1;
  }

  if ((errcode=gras_rawsock_server_open(6666,8000,msgSize,&raw))) { 
    fprintf(stderr,"cbSatBegin(): Error %s encountered while opening a raw socket\n",
	    xbt_error_name(errcode));
    grasRepportError(msg->sock,GRASMSG_SAT_BEGUN,2,
		     "cbSatBegin: Severe error: Cannot send error status to requester!!\n",
		     errcode,"Cannot open raw socket");
    return 1;
  }
  request->port=gras_rawsock_get_peer_port(raw);
  request->msgSize=msgSize;
  error->errcode=no_error;
  error->errmsg[0]='\0';
  if ((errcode=gras_msg_new_and_send(msg->sock,GRASMSG_SAT_BEGUN,2,
			      error,1,
			      request,1))) {
    fprintf(stderr,"cbSatBegin(): Error %s encountered while send ACK to peer\n",
	    xbt_error_name(errcode));
    return 1;
  }
  gras_msg_free(msg);

  start=gras_time();
  while (gras_msg_wait(0,GRASMSG_SAT_END,&msg)==timeout_error &&
	 gras_time() - start < timeout) {
    errcode=gras_rawsock_recv(raw,msgSize,msgSize,1);
    if (errcode != timeout_error && errcode != no_error) {
      fprintf(stderr,"cbSatBegin: Failure %s during raw receive\n",xbt_error_name(errcode));
      /* our error message do not interess anyone. SAT_END will do nothing. */
      /* (if timeout'ed, it may be because the sender stopped emission. so survive it) */
      return 1;
    } 
  }
  if (gras_time()-start > timeout) {
    fprintf(stderr,"The saturation experiment did timeout. Stop it NOW.\n");
    gras_rawsock_close(raw);
    return 1;
  }

  grasRepportError(msg->sock,GRASMSG_SAT_ENDED,1,
		   "cbSatBegin: Cannot send SAT_ENDED.\n",
		   no_error,"");
  gras_rawsock_close(raw);
  gras_msg_free(msg);
  return 1;
}

xbt_error_t grasbw_saturate_stop(const char* from_name,unsigned int from_port,
				 const char* to_name,unsigned int to_port) {
  xbt_error_t errcode;
  gras_sock_t *sock;
  gras_msg_t *answer;

  if((errcode=gras_sock_client_open(from_name,from_port,&sock))) {
    fprintf(stderr,"saturate_stop(): Error %s encountered while contacting peer\n",
	    xbt_error_name(errcode));
    return errcode;
  }

  if ((errcode=gras_msg_new_and_send(sock,GRASMSG_SAT_STOP,0))) {
    fprintf(stderr,"saturate_stop(): Error %s encountered while sending request\n",
	    xbt_error_name(errcode));
    gras_sock_close(sock);
    return errcode;
  }

  if ((errcode=gras_msg_wait(120,GRASMSG_SAT_STOPPED,&answer))) {
    fprintf(stderr,"saturate_stop(): Error %s encountered while receiving ACK\n",
	    xbt_error_name(errcode));
    gras_sock_close(sock);
    return errcode;
  }

  if((errcode=gras_msg_ctn(answer,0,0,msgError_t).errcode)) {
    fprintf(stderr,"saturate_stop(): Peer reported error %s (%s).\n",
	    xbt_error_name(errcode),gras_msg_ctn(answer,0,0,msgError_t).errmsg);
    gras_msg_free(answer);
    gras_sock_close(sock);
    return errcode;
  }

  gras_msg_free(answer);
  gras_sock_close(sock);

  return no_error;
}
#endif
