/* $Id$ */

/* amok_saturate - Link saturating facilities (for ALNeM's BW testing)      */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003, 2004 the OURAGAN project.                            */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "amok/Bandwidth/bandwidth_private.h"

GRAS_LOG_EXTERNAL_CATEGORY(bw);
GRAS_LOG_DEFAULT_CATEGORY(bw);

#if 0
/* ***************************************************************************
 * Link saturation
 * ***************************************************************************/

gras_error_t grasbw_saturate_start(const char* from_name,unsigned int from_port,
				   const char* to_name,unsigned int to_port,
				   unsigned int msgSize, unsigned int timeout) {
  gras_sock_t *sock;
  gras_error_t errcode;
  /* The request */
  SatExp_t *request;
  msgHost_t *target;
  /* answer */
  gras_msg_t *answer;

  if((errcode=gras_sock_client_open(from_name,from_port,&sock))) {
    fprintf(stderr,"%s:%d:saturate_start(): Error %s encountered while contacting peer\n",
	    __FILE__,__LINE__,gras_error_name(errcode));
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
	    __FILE__,__LINE__,gras_error_name(errcode));
    gras_sock_close(sock);
    return errcode;
  }
  if ((errcode=gras_msg_wait(120,GRASMSG_SAT_STARTED,&answer))) {
    fprintf(stderr,"%s:%d:saturate_start(): Error %s encountered while waiting for the ACK.\n",
	    __FILE__,__LINE__,gras_error_name(errcode));
    gras_sock_close(sock);
    return errcode;
  }

  if((errcode=gras_msg_ctn(answer,0,0,msgError_t).errcode)) {
    fprintf(stderr,"%s:%d:saturate_start(): Peer reported error %s (%s).\n",
	    __FILE__,__LINE__,gras_error_name(errcode),gras_msg_ctn(answer,0,0,msgError_t).errmsg);
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
  gras_error_t errcode;
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
	    gras_error_name(errcode));
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
	    gras_error_name(errcode));
    grasRepportError(msg->sock,GRASMSG_SAT_STARTED,1,
		     "cbSatStart: Severe error: Cannot send error status to requester!!\n",
		     errcode,"Cannot send request.\n");
    gras_sock_close(sock);
    return 1;
  }

  if ((errcode=gras_msg_wait(120,GRASMSG_SAT_BEGUN,&answer))) {
    fprintf(stderr,"cbSatStart(): Error %s encountered while waiting for the ACK.\n",
	    gras_error_name(errcode));
    gras_sock_close(sock);

    grasRepportError(msg->sock,GRASMSG_SAT_STARTED,1,
		     "cbSatStart: Severe error: Cannot send error status to requester!!\n",
		     errcode,
		     "Cannot receive the ACK.\n");
    return 1;
  }

  if((errcode=gras_msg_ctn(answer,0,0,msgError_t).errcode)) {
    fprintf(stderr,"cbSatStart(): Peer reported error %s (%s).\n",
	    gras_error_name(errcode),gras_msg_ctn(answer,0,0,msgError_t).errmsg);

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
	    gras_error_name(errcode),to_name,gras_msg_ctn(answer,1,0,SatExp_t).port);

    grasRepportError(msg->sock,GRASMSG_SAT_STARTED,1,
		     "cbSatStart: Severe error: Cannot send error status to requester!!\n",
		     errcode,"Cannot open raw socket.\n");
    gras_sock_close(sock);
    return 1;
  }

  /* send a train of data before repporting that XP is started */
  if ((errcode=gras_rawsock_send(raw,msgSize,msgSize))) {
    fprintf(stderr,"cbSatStart: Failure %s during raw send\n",gras_error_name(errcode));
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
      fprintf(stderr,"cbSatStart: Failure %s during raw send\n",gras_error_name(errcode));
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
  gras_error_t errcode;
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
	    gras_error_name(errcode));
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
	    gras_error_name(errcode));
    return 1;
  }
  gras_msg_free(msg);

  start=gras_time();
  while (gras_msg_wait(0,GRASMSG_SAT_END,&msg)==timeout_error &&
	 gras_time() - start < timeout) {
    errcode=gras_rawsock_recv(raw,msgSize,msgSize,1);
    if (errcode != timeout_error && errcode != no_error) {
      fprintf(stderr,"cbSatBegin: Failure %s during raw receive\n",gras_error_name(errcode));
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

gras_error_t grasbw_saturate_stop(const char* from_name,unsigned int from_port,
				 const char* to_name,unsigned int to_port) {
  gras_error_t errcode;
  gras_sock_t *sock;
  gras_msg_t *answer;

  if((errcode=gras_sock_client_open(from_name,from_port,&sock))) {
    fprintf(stderr,"saturate_stop(): Error %s encountered while contacting peer\n",
	    gras_error_name(errcode));
    return errcode;
  }

  if ((errcode=gras_msg_new_and_send(sock,GRASMSG_SAT_STOP,0))) {
    fprintf(stderr,"saturate_stop(): Error %s encountered while sending request\n",
	    gras_error_name(errcode));
    gras_sock_close(sock);
    return errcode;
  }

  if ((errcode=gras_msg_wait(120,GRASMSG_SAT_STOPPED,&answer))) {
    fprintf(stderr,"saturate_stop(): Error %s encountered while receiving ACK\n",
	    gras_error_name(errcode));
    gras_sock_close(sock);
    return errcode;
  }

  if((errcode=gras_msg_ctn(answer,0,0,msgError_t).errcode)) {
    fprintf(stderr,"saturate_stop(): Peer reported error %s (%s).\n",
	    gras_error_name(errcode),gras_msg_ctn(answer,0,0,msgError_t).errmsg);
    gras_msg_free(answer);
    gras_sock_close(sock);
    return errcode;
  }

  gras_msg_free(answer);
  gras_sock_close(sock);

  return no_error;
}
#endif
