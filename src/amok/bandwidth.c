/* $Id$ */

/* gras_bandwidth - GRAS mecanism to do Bandwidth tests between to hosts    */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gras.h>

/**
 * BwExp_t:
 *
 * Description of a BW experiment (payload when asking an host to do a BW experiment with us)
 */
typedef struct {
  unsigned int bufSize;
  unsigned int expSize;
  unsigned int msgSize;
  unsigned int port;    /* raw socket to use */
} BwExp_t;

static const DataDescriptor BwExp_Desc[] = 
 { SIMPLE_MEMBER(UNSIGNED_INT_TYPE, 1, offsetof(BwExp_t,bufSize)),
   SIMPLE_MEMBER(UNSIGNED_INT_TYPE, 1, offsetof(BwExp_t,expSize)),
   SIMPLE_MEMBER(UNSIGNED_INT_TYPE, 1, offsetof(BwExp_t,msgSize)),
   SIMPLE_MEMBER(UNSIGNED_INT_TYPE, 1, offsetof(BwExp_t,port))};
#define BwExp_Len 4

/**
 * SatExp_t:
 *
 * Description of a BW experiment (payload when asking an host to do a BW experiment with us)
 */
typedef struct {
  unsigned int msgSize;
  unsigned int timeout;
  unsigned int port;    /* raw socket to use */
} SatExp_t;

static const DataDescriptor SatExp_Desc[] = 
 { SIMPLE_MEMBER(UNSIGNED_INT_TYPE, 1, offsetof(SatExp_t,msgSize)),
   SIMPLE_MEMBER(UNSIGNED_INT_TYPE, 1, offsetof(SatExp_t,timeout)),
   SIMPLE_MEMBER(UNSIGNED_INT_TYPE, 1, offsetof(SatExp_t,port))};
#define SatExp_Len 3


/* Prototypes of local callbacks */
int grasbw_cbBWHandshake(gras_msg_t *msg);
int grasbw_cbBWRequest(gras_msg_t *msg);

int grasbw_cbSatStart(gras_msg_t *msg);
int grasbw_cbSatBegin(gras_msg_t *msg);

/**** code ****/
gras_error_t grasbw_register_messages(void) {
  gras_error_t errcode;

  
  if ( /* Bandwidth */
      (errcode=gras_msgtype_register(GRASMSG_BW_REQUEST,"BW request",2,
			       msgHostDesc,msgHostLen,
			       BwExp_Desc,BwExp_Len)) ||
      (errcode=gras_msgtype_register(GRASMSG_BW_RESULT, "BW result",2,
			       msgErrorDesc,msgErrorLen,
			       msgResultDesc,msgResultLen /* first=seconds, second=bw */)) ||
      
      (errcode=gras_msgtype_register(GRASMSG_BW_HANDSHAKE, "BW handshake",1,
			       BwExp_Desc,BwExp_Len))  || 
      (errcode=gras_msgtype_register(GRASMSG_BW_HANDSHAKED, "BW handshake ACK",1,
			       BwExp_Desc,BwExp_Len)) ||
      
      /* Saturation */
      (errcode=gras_msgtype_register(GRASMSG_SAT_START,"SAT_START",2,
			       msgHostDesc,msgHostLen,
			       SatExp_Desc,SatExp_Len)) ||
      (errcode=gras_msgtype_register(GRASMSG_SAT_STARTED, "SAT_STARTED",1,
			       msgErrorDesc,msgErrorLen)) ||
      
      (errcode=gras_msgtype_register(GRASMSG_SAT_BEGIN,"SAT_BEGIN",1,
			       SatExp_Desc,SatExp_Len)) ||
      (errcode=gras_msgtype_register(GRASMSG_SAT_BEGUN, "SAT_BEGUN",2,
			       msgErrorDesc,msgErrorLen,
			       SatExp_Desc,SatExp_Len)) ||
      
      (errcode=gras_msgtype_register(GRASMSG_SAT_END,"SAT_END",0)) ||
      (errcode=gras_msgtype_register(GRASMSG_SAT_ENDED, "SAT_ENDED",1,
			       msgErrorDesc,msgErrorLen)) ||
      
      (errcode=gras_msgtype_register(GRASMSG_SAT_STOP,"SAT_STOP",0)) ||
      (errcode=gras_msgtype_register(GRASMSG_SAT_STOPPED, "SAT_STOPPED",1,
			       msgErrorDesc,msgErrorLen)) ) 
    {

      fprintf(stderr,"GRASBW: Unable register the messages (got error %s)\n",
	      gras_error_name(errcode));
      return errcode;
    }

  if ((errcode=gras_cb_register(GRASMSG_BW_HANDSHAKE,-1,&grasbw_cbBWHandshake)) ||
      (errcode=gras_cb_register(GRASMSG_BW_REQUEST,-1,&grasbw_cbBWRequest))  ||

      (errcode=gras_cb_register(GRASMSG_SAT_START,-1,&grasbw_cbSatStart))  ||
      (errcode=gras_cb_register(GRASMSG_SAT_BEGIN,-1,&grasbw_cbSatBegin)) ) {

    fprintf(stderr,"GRASBW: Unable register the callbacks (got error %s)\n",
	    gras_error_name(errcode));
    return errcode;
  }

  return no_error;
}

/* ***************************************************************************
 * Bandwidth tests
 * ***************************************************************************/
/* Function to do a test from local to given host */
gras_error_t grasbw_test(const char*to_name,unsigned int to_port,
			unsigned int bufSize,unsigned int expSize,unsigned int msgSize,
			/*OUT*/ double *sec, double *bw) {
  gras_rawsock_t *rawIn,*rawOut;
  gras_sock_t *sock;
  gras_error_t errcode;
  BwExp_t *request;
  gras_msg_t *answer;
  
  if((errcode=gras_sock_client_open(to_name,to_port,&sock))) {
    fprintf(stderr,"grasbw_test(): Error %s encountered while contacting peer\n",
	    gras_error_name(errcode));
    return errcode;
  }
  if ((errcode=gras_rawsock_server_open(6666,8000,bufSize,&rawIn))) { 
    fprintf(stderr,"grasbw_test(): Error %s encountered while opening a raw socket\n",
	    gras_error_name(errcode));
    return errcode;
  }

  if (!(request=(BwExp_t *)malloc(sizeof(BwExp_t)))) {
    fprintf(stderr,"grasbw_test(): Malloc error\n");
    gras_sock_close(sock);
    return malloc_error;    
  }
  request->bufSize=bufSize;
  request->expSize=expSize;
  request->msgSize=msgSize;
  request->port=gras_rawsock_get_peer_port(rawIn);

  if ((errcode=gras_msg_new_and_send(sock,GRASMSG_BW_HANDSHAKE, 1, 
			      request,1))) {
    fprintf(stderr,"grasbw_test(): Error %s encountered while sending the request.\n",
	    gras_error_name(errcode));
    gras_sock_close(sock);
    return errcode;
  }
  if ((errcode=gras_msg_wait(60,GRASMSG_BW_HANDSHAKED,&answer))) {
    fprintf(stderr,"grasbw_test(): Error %s encountered while waiting for the answer.\n",
	    gras_error_name(errcode));
    gras_sock_close(sock);
    return errcode;
  }
  if((errcode=gras_rawsock_client_open(to_name,gras_msg_ctn(answer,0,0,BwExp_t).port,
				    bufSize,&rawOut))) {
    fprintf(stderr,"grasbw_test(): Error %s encountered while opening the raw socket to %s:%d\n",
	    gras_error_name(errcode),to_name,gras_msg_ctn(answer,0,0,BwExp_t).port);
    return errcode;
  }

  *sec=gras_time();
  if ((errcode=gras_rawsock_send(rawOut,expSize,msgSize)) ||
      (errcode=gras_rawsock_recv(rawIn,1,1,120))) {
    fprintf(stderr,"grasbw_test(): Error %s encountered while sending the experiment.\n",
	    gras_error_name(errcode));
    gras_rawsock_close(rawOut);
    gras_rawsock_close(rawIn);
    return errcode;
  }
  *sec = gras_time() - *sec;
  *bw = ((double)expSize /* 8.0*/) / *sec / (1024.0 *1024.0);

  gras_rawsock_close(rawIn);
  gras_rawsock_close(rawOut);
  gras_sock_close(sock);
  gras_msg_free(answer);
  return no_error;
}

/* Callback to the GRASMSG_BW_HANDSHAKE message: 
   opens a server raw socket,
   indicate its port in an answer GRASMSG_BW_HANDSHAKED message,
   receive the corresponding data on the raw socket, 
   close the raw socket
*/
int grasbw_cbBWHandshake(gras_msg_t *msg) {
  gras_rawsock_t *rawIn,*rawOut;
  BwExp_t *ans;
  gras_error_t errcode;
  
  if ((errcode=gras_rawsock_server_open(6666,8000,gras_msg_ctn(msg,0,0,BwExp_t).bufSize,&rawIn))) { 
    fprintf(stderr,"grasbw_cbHandshake(): Error %s encountered while opening a raw socket\n",
	    gras_error_name(errcode));
    return 1;
  }
  if ((errcode=gras_rawsock_client_open(gras_sock_get_peer_name(msg->sock),gras_msg_ctn(msg,0,0,BwExp_t).port,
				     gras_msg_ctn(msg,0,0,BwExp_t).bufSize,&rawOut))) { 
    fprintf(stderr,"grasbw_cbHandshake(): Error %s encountered while opening a raw socket\n",
	    gras_error_name(errcode));
    return 1;
  }
  if (!(ans=(BwExp_t *)malloc(sizeof(BwExp_t)))) {
    fprintf(stderr,"grasbw_cbHandshake(): Malloc error.\n");
    gras_rawsock_close(rawIn);
    gras_rawsock_close(rawOut);
    return 1;
  }
  ans->bufSize=gras_msg_ctn(msg,0,0,BwExp_t).bufSize;
  ans->expSize=gras_msg_ctn(msg,0,0,BwExp_t).expSize;
  ans->msgSize=gras_msg_ctn(msg,0,0,BwExp_t).msgSize;
  ans->port=gras_rawsock_get_peer_port(rawIn);
  //  fprintf(stderr,"grasbw_cbHandshake. bufSize=%d expSize=%d msgSize=%d port=%d\n",
  //	  ans->bufSize,ans->expSize,ans->msgSize,ans->port);

  if ((errcode=gras_msg_new_and_send(msg->sock,GRASMSG_BW_HANDSHAKED, 1,
			      ans, 1))) {
    fprintf(stderr,"grasbw_cbHandshake(): Error %s encountered while sending the answer.\n",
	    gras_error_name(errcode));
    gras_rawsock_close(rawIn);
    gras_rawsock_close(rawOut);
    return 1;
  }
    
  if ((errcode=gras_rawsock_recv(rawIn,
			       gras_msg_ctn(msg,0,0,BwExp_t).expSize,
			       gras_msg_ctn(msg,0,0,BwExp_t).msgSize,
			       120)) ||
      (errcode=gras_rawsock_send(rawOut,1,1))) {
    fprintf(stderr,"grasbw_cbHandshake(): Error %s encountered while receiving the experiment.\n",
	    gras_error_name(errcode));
    gras_rawsock_close(rawIn);
    gras_rawsock_close(rawOut);
    return 1;
  }
  gras_msg_free(msg);
  gras_rawsock_close(rawIn);
  gras_rawsock_close(rawOut);
  return 1;
}

/* function to request a BW test between to external hosts */
gras_error_t grasbw_request(const char* from_name,unsigned int from_port,
			   const char* to_name,unsigned int to_port,
			   unsigned int bufSize,unsigned int expSize,unsigned int msgSize,
			   /*OUT*/ double *sec, double*bw) {
  
  gras_sock_t *sock;
  gras_msg_t *answer;
  gras_error_t errcode;
  /* The request */
  BwExp_t *request;
  msgHost_t *target;

  if((errcode=gras_sock_client_open(from_name,from_port,&sock))) {
    fprintf(stderr,"grasbw_request(): Error %s encountered while contacting the actuator\n",
	    gras_error_name(errcode));
    return errcode;
  }
  if (!(request=(BwExp_t *)malloc(sizeof(BwExp_t))) ||
      !(target=(msgHost_t*)malloc(sizeof(msgHost_t)))) {
    fprintf(stderr,"grasbw_test(): Malloc error\n");
    gras_sock_close(sock);
    return malloc_error;    
  }

  request->bufSize=bufSize;
  request->expSize=expSize;
  request->msgSize=msgSize;
  strcpy(target->host,to_name);
  target->port=to_port;
  
  if ((errcode=gras_msg_new_and_send(sock,GRASMSG_BW_REQUEST, 2, 
			      target,1,
			      request,1))) {
    fprintf(stderr,"grasbw_request(): Error %s encountered while sending the request.\n",
	    gras_error_name(errcode));
    gras_sock_close(sock);
    return errcode;
  }
  if ((errcode=gras_msg_wait(240,GRASMSG_BW_RESULT,&answer))) {
    fprintf(stderr,"grasbw_request(): Error %s encountered while waiting for the answer.\n",
	    gras_error_name(errcode));
    gras_sock_close(sock);
    return errcode;
  }

  if((errcode=gras_msg_ctn(answer,0,0,msgError_t).errcode)) {
    fprintf(stderr,"grasbw_request(): Peer reported error %s (%s).\n",
	    gras_error_name(errcode),gras_msg_ctn(answer,0,0,msgError_t).errmsg);
    gras_msg_free(answer);
    gras_sock_close(sock);
    return errcode;
  }

  //  fprintf(stderr,"sec=%p",gras_msg_ctn(answer,1,0,msgResult_t));
  *sec=gras_msg_ctn(answer,1,0,msgResult_t).value;
  *bw=gras_msg_ctn(answer,1,1,msgResult_t).value;

  gras_msg_free(answer);
  gras_sock_close(sock);
  return no_error;
}

int grasbw_cbBWRequest(gras_msg_t *msg) {
  /* specification of the test to run */
  char* to_name=gras_msg_ctn(msg,0,0,msgHost_t).host;
  unsigned int to_port=gras_msg_ctn(msg,0,0,msgHost_t).port;

  unsigned int bufSize=gras_msg_ctn(msg,1,0,BwExp_t).bufSize;
  unsigned int expSize=gras_msg_ctn(msg,1,0,BwExp_t).expSize;
  unsigned int msgSize=gras_msg_ctn(msg,1,0,BwExp_t).msgSize;
  /* our answer */
  msgError_t *error;
  msgResult_t *res;

  if (!(error=(msgError_t *)malloc(sizeof(msgError_t))) ||
      !(res=(msgResult_t *)malloc(sizeof(msgResult_t) * 2))) {
    fprintf(stderr,"%s:%d:grasbw_cbRequest: Malloc error\n",__FILE__,__LINE__);
    return malloc_error;    
  }

  if ((error->errcode=grasbw_test(to_name,to_port,bufSize,expSize,msgSize,
				  &(res[0].value),&(res[1].value) ))) {
    fprintf(stderr,
	    "%s:%d:grasbw_cbRequest: Error %s encountered while doing the test\n",
	    __FILE__,__LINE__,gras_error_name(error->errcode));
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
}

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
