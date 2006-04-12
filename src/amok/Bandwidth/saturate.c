/* $Id$ */

/* amok_saturate - Link saturating facilities (for ALNeM's BW testing)      */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "amok/Bandwidth/bandwidth_private.h"
#include "gras/Msg/msg_private.h" /* FIXME: This mucks with contextes to answer RPC directly */

XBT_LOG_EXTERNAL_CATEGORY(bw);
XBT_LOG_DEFAULT_CATEGORY(bw);

static int amok_bw_cb_sat_start(gras_msg_cb_ctx_t ctx, void *payload);
static int amok_bw_cb_sat_begin(gras_msg_cb_ctx_t ctx, void *payload);


void amok_bw_sat_init(void) {
  gras_datadesc_type_t sat_request_desc;
  /* Build the saturation datatype descriptions */ 
  
  sat_request_desc = gras_datadesc_struct("s_sat_request_desc_t");
  gras_datadesc_struct_append(sat_request_desc,"host",gras_datadesc_by_name("s_xbt_host_t"));
  gras_datadesc_struct_append(sat_request_desc,"msg_size",gras_datadesc_by_name("unsigned int"));
  gras_datadesc_struct_append(sat_request_desc,"duration",gras_datadesc_by_name("unsigned int"));
  gras_datadesc_struct_close(sat_request_desc);
  sat_request_desc = gras_datadesc_ref("sat_request_t",sat_request_desc);
  
  /* Register the saturation messages */
  gras_msgtype_declare_rpc("amok_bw_sat start", sat_request_desc, NULL);
  gras_msgtype_declare_rpc("amok_bw_sat begin", sat_request_desc, sat_request_desc);
  gras_msgtype_declare_rpc("amok_bw_sat stop",  NULL,             NULL);

}
void amok_bw_sat_join(void) {
  gras_cb_register(gras_msgtype_by_name("amok_bw_sat start"),
		   &amok_bw_cb_sat_start);
  gras_cb_register(gras_msgtype_by_name("amok_bw_sat begin"),
		   &amok_bw_cb_sat_begin);
}
void amok_bw_sat_leave(void) {
  gras_cb_unregister(gras_msgtype_by_name("amok_bw_sat start"),
		     &amok_bw_cb_sat_start);
  gras_cb_unregister(gras_msgtype_by_name("amok_bw_sat begin"),
		     &amok_bw_cb_sat_begin);
}

/* ***************************************************************************
 * Link saturation
 * ***************************************************************************/

/**
 * @brief Ask 'from_name:from_port' to stop saturating going to to_name:to_name.
 *
 * @param from_name: Name of the host we are asking to do a experiment with (to_name:to_port)
 * @param from_port: port on which the process we are asking for an experiment is listening
 * (for message, do not give a raw socket here. The needed raw socket will be negociated 
 * between the peers)
 * @param to_name: Name of the host with which we should conduct the experiment
 * @param to_port: port on which the peer process is listening for message
 * @param msg_size: Size of each message sent.
 * @param duration: How long in maximum should be the saturation.
 *
 * Ask the process 'from_name:from_port' to start to saturate the link between itself
 * and to_name:to_name.
 */
void amok_bw_saturate_start(const char* from_name,unsigned int from_port,
			    const char* to_name,unsigned int to_port,
			    unsigned int msg_size, double duration) {
  gras_socket_t sock;

  sat_request_t request = xbt_new(s_sat_request_t,1);

  sock = gras_socket_client(from_name,from_port);

  request->host.name = (char*)to_name;
  request->host.port = to_port;
  
  request->duration=duration;
  request->msg_size=msg_size;


  gras_msg_rpccall(sock,60,gras_msgtype_by_name("amok_bw_sat start"),&request, NULL);

  free(request);
  gras_socket_close(sock);
}

/* Asked to begin a saturation */
static int amok_bw_cb_sat_start(gras_msg_cb_ctx_t ctx, void *payload){
  sat_request_t request = *(sat_request_t*)payload;
  gras_msg_rpcreturn(60,ctx, NULL);
  amok_bw_saturate_begin(request->host.name,request->host.port,
			 request->msg_size, request->duration,
			 NULL,NULL);
  free(request->host.name);
  free(request);
  return 1;
}

/**
 * @brief Start saturating between the current process and the designated peer
 *
 * Note that the only way to break this function before the end of the timeout
 * is to have a remote host calling amok_bw_saturate_stop to this process.
 */
void amok_bw_saturate_begin(const char* to_name,unsigned int to_port,
			    unsigned int msg_size, double duration,
			    /*out*/ double *elapsed_res, double *bw_res) {
 
  xbt_ex_t e;

  gras_socket_t peer_cmd = gras_socket_client(to_name, to_port);
  gras_msg_cb_ctx_t ctx;

  gras_socket_t meas;

  s_gras_msg_t msg_got;

  unsigned int packet_sent=0;
  double start,elapsed=-1; /* timer */
  double bw;

  volatile int saturate_further; /* boolean in the main loop */ 

  /* Negociate the saturation with the peer */
  sat_request_t request = xbt_new(s_sat_request_t,1);

  DEBUG2("Begin to saturate to %s:%d",to_name,to_port);
  memset(&msg_got,0,sizeof(msg_got));

  request->msg_size = msg_size;
  request->duration = duration;
  request->host.name = NULL;
  request->host.port = 0;

  ctx = gras_msg_rpc_async_call(peer_cmd, 60, 
				gras_msgtype_by_name("amok_bw_sat begin"),
						  &request);
  free(request);
  gras_msg_rpc_async_wait(ctx,&request);
  meas=gras_socket_client_ext( to_name, request->host.port,
			       0 /*bufsize: auto*/,
			       1 /*meas: true*/);
  free(request);

  gras_socket_close(peer_cmd);
  INFO2("Saturation from %s to %s started",gras_os_myname(),to_name);

  /* Start experiment */
  start=gras_os_time();

  do {
    /* do send it */
    gras_socket_meas_send(meas,120,msg_size,msg_size);
    packet_sent++;

    /* Check whether someone asked us to stop saturation */
    saturate_further = 0;
    TRY {
      gras_msg_wait_ext(0/*no wait*/,gras_msgtype_by_name("amok_bw_sat stop"),
			NULL /* accept any sender */,
			NULL, NULL, /* No specific filter */
			&msg_got);
    } CATCH(e) {
      if (e.category == timeout_error) {
	saturate_further=1;
	memset(&msg_got,0,sizeof(msg_got)); /* may be overprotectiv here */
      }
      xbt_ex_free(&e);
    }

    /* Check whether the experiment has to be terminated by now */
    elapsed=gras_os_time()-start;
    VERB2("elapsed %f duration %f",elapsed, duration);

  } while (saturate_further && elapsed < duration);

  INFO2("Saturation from %s to %s stopped",gras_os_myname(),to_name);
  bw = ((double)(packet_sent*msg_size)) / elapsed;

  if (elapsed_res)
    *elapsed_res = elapsed;
  if (bw_res)
    *bw_res = bw;

  if (elapsed >= duration) {
    INFO2("Saturation experiment terminated. Took %f sec (achieving %f kb/s)",
	  elapsed, bw/1024.0);
  }

  /* If someone stopped us, inform him about the achieved bandwidth */
  if (msg_got.expe) {
    bw_res_t answer = xbt_new(s_bw_res_t,1);
    s_gras_msg_cb_ctx_t ctx;

    answer->timestamp=gras_os_time();
    answer->sec=elapsed;
    answer->bw=bw;

    ctx.expeditor = msg_got.expe;
    ctx.ID = msg_got.ID;
    ctx.msgtype = msg_got.type;

    gras_msg_rpcreturn(60,&ctx,&answer);
    free(answer);
  }
  
  gras_socket_close(meas);
}

/* Sender will saturate link to us */
static int amok_bw_cb_sat_begin(gras_msg_cb_ctx_t ctx, void *payload){
  sat_request_t request=*(sat_request_t*)payload;
  sat_request_t answer=xbt_new0(s_sat_request_t,1);
  volatile int saturate_further = 1;
  xbt_ex_t e;
  gras_socket_t measMaster=NULL,meas=NULL;

  int port=6000;
  while (port <= 10000 && measMaster == NULL) {
    TRY {
      measMaster = gras_socket_server_ext(port,
					  0 /*bufsize: auto*/,
					  1 /*meas: true*/);
    } CATCH(e) {
      measMaster = NULL;
      if (port < 10000)
	xbt_ex_free(&e);
      else
	RETHROW0("Error encountered while opening a measurement server socket: %s");
    }
    if (measMaster == NULL) 
      port++; /* prepare for a new loop */
  }
  answer->host.port=port;

  gras_msg_rpcreturn(60, ctx, &answer);
  free(answer);

  gras_os_sleep(5); /* Wait for the accept */

  TRY {
    meas = gras_socket_meas_accept(measMaster);
    DEBUG0("saturation handshake answered");
  } CATCH(e) {
    gras_socket_close(measMaster);
    RETHROW0("Error during saturation handshake: %s");
  }  

  while (saturate_further) {
    TRY {
      gras_socket_meas_recv(meas,120,request->msg_size,request->msg_size);
    } CATCH(e) {
      saturate_further = 0;
      xbt_ex_free(&e);
    }
  }
  INFO1("Saturation stopped on %s",gras_os_myname());
  gras_socket_close(meas);
  if (gras_if_RL()) /* On SG, accepted=master */
    gras_socket_close(measMaster); 
  free(request);
  return 1;
}

/**
 * @brief Ask 'from_name:from_port' to stop any saturation experiments
 * @param from_name: Name of the host we are asking to do a experiment with (to_name:to_port)
 * @param from_port: port on which the process we are asking for an experiment is listening
 * @param time: the duration of the experiment
 * @param bw: the achieved bandwidth
 *
 */
void amok_bw_saturate_stop(const char* from_name,unsigned int from_port,
			   /*out*/ unsigned int *time, unsigned int *bw) {

  gras_socket_t sock = gras_socket_client(from_name,from_port);
  gras_msg_rpccall(sock,60,gras_msgtype_by_name("amok_bw_sat stop"),NULL,NULL);
  gras_socket_close(sock);
}


#if 0
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
