/* $Id$ */

/* gras_rl - implementation of GRAS on real life                            */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras_rl.h"

#include <stdlib.h>
#include <string.h>

#include <unistd.h> /* sleep() */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> /* struct in_addr */
#include <signal.h>


/* NWS headers */
#include "osutil.h"
#include "timeouts.h"
#include "protocol.h"

GRAS_LOG_NEW_DEFAULT_CATEGORY(rl);
			      
/* globals */
static grasProcessData_t *_grasProcessData;

/* Prototypes of internal functions */
static int grasConversionRequired(const DataDescriptor *description, size_t howMany);
static gras_error_t
_gras_rawsock_exchange(gras_rawsock_t *sd, int sender, unsigned int timeout,
		       unsigned int expSize, unsigned int msgSize);


gras_error_t gras_process_init() {
  if (!(_grasProcessData=(grasProcessData_t *)malloc(sizeof(grasProcessData_t)))) {
    fprintf(stderr,"gras_process_init: cannot malloc %d bytes\n",sizeof(grasProcessData_t));
    return malloc_error;
  }
  _grasProcessData->grasMsgQueueLen=0;
  _grasProcessData->grasMsgQueue = NULL;

  _grasProcessData->grasCblListLen = 0;
  _grasProcessData->grasCblList = NULL;

  _grasProcessData->userdata = NULL;
  return no_error;
}
gras_error_t gras_process_finalize() {
  fprintf(stderr,"FIXME: %s not implemented (=> leaking on exit :)\n",__FUNCTION__);
  return no_error;
}

/* **************************************************************************
 * Openning/Maintaining/Closing connexions
 * **************************************************************************/
gras_error_t
gras_sock_client_open(const char *host, short port, 
		      /* OUT */ gras_sock_t **sock) {

  int addrCount;
  IPAddress addresses[10];
  int i;
  int sd;
  
  if (!(*sock=malloc(sizeof(gras_sock_t)))) {
    fprintf(stderr,"Malloc error\n");
    return malloc_error;
  }
  (*sock)->peer_addr=NULL;

  if (!(addrCount = IPAddressValues(host, addresses, 10))) {
    fprintf(stderr,"grasOpenClientSocket: address retrieval of '%s' failed\n",host);
    return system_error;
  }

  for(i = 0; i < addrCount && i<10 ; i++) {
    if(CallAddr(addresses[i], port, &sd, -1)) {
      (*sock)->sock = sd;
      (*sock)->port = port;
      return no_error;
    }
  }
  free(*sock);
  fprintf(stderr,"grasOpenClientSocket: something wicked happenned while connecting to %s:%d",
	  host,port);
  return system_error;
}


gras_error_t
gras_sock_server_open(unsigned short startingPort, unsigned short endingPort,
		      /* OUT */ gras_sock_t **sock) {

  unsigned short port;

  if (!(*sock=malloc(sizeof(gras_sock_t)))) {
    fprintf(stderr,"Malloc error\n");
    return malloc_error;
  }
  
  if (!EstablishAnEar(startingPort,endingPort,&((*sock)->sock),&port)) {
    free(*sock);
    return unknown_error;
  }
  (*sock)->peer_addr=NULL;
  (*sock)->port=port;

  return no_error;
}

gras_error_t gras_sock_close(gras_sock_t *sock) {
  if (sock) {
    DROP_SOCKET(&(sock->sock));
    if (sock->peer_addr) free(sock->peer_addr);
    free(sock);
  }
  return no_error;
}

unsigned short
gras_sock_get_my_port(gras_sock_t *sd) {
  if (!sd) return -1;
  return sd->port;
}

unsigned short
gras_sock_get_peer_port(gras_sock_t *sd) {
  if (!sd) return -1;
  return PeerNamePort(sd->sock);
}

char *
gras_sock_get_peer_name(gras_sock_t *sd) {
  char *tmp;

  if (!sd) return NULL;
  tmp=PeerName_r(sd->sock);
  if (tmp) {
    strcpy(sd->peer_name,tmp);
    free(tmp);
  } else {
    strcpy(sd->peer_name,"unknown");
  }

  return sd->peer_name;
}


/* **************************************************************************
 * format handling
 * **************************************************************************/

/*
 * Returns 1 or 0 depending on whether or not format conversion is required for
 * data with the format described by the #howMany#-long array #description#.
 */
int grasConversionRequired(const DataDescriptor *description, size_t howMany){
  int i;
  
  if(DataSize(description, howMany, HOST_FORMAT) !=
     DataSize(description, howMany, NETWORK_FORMAT)) {
    return 1;
  }
  
  for(i = 0; i < howMany; i++) {
    if(description[i].type == STRUCT_TYPE) {
      if(grasConversionRequired(description[i].members, description[i].length)) {
	return 1;
      }
    } else if(DifferentFormat(description[i].type))
      return 1;
  }
  
  return DifferentOrder();
}

/* **************************************************************************
 * Actually exchanging messages
 * **************************************************************************/

/*
 * Discard data on the socket because of failure on our side
 */
void
gras_msg_discard(gras_sock_t *sd, size_t size) {
  char garbage[2048];
  int s=size;

  while(s > 0) {
    (void)RecvBytes(sd->sock, 
		    garbage, 
		    (s > sizeof(garbage)) ? sizeof(garbage) : s,
		    GetTimeOut(RECV, Peer(sd->sock), 2048));
    s -= sizeof(garbage);
  }
}

int grasDataRecv( gras_sock_t *sd,
		  void **data,
		  const DataDescriptor *description,
		  size_t description_length,
		  unsigned int repetition) {
  
  void *converted=NULL;
  int convertIt;
  void *destination; /* where to receive the data from the net (*data or converted)*/
  int recvResult;
  size_t netSize,hostSize;
  double start; /* for timeouts */
  char *net,*host; /* iterators */
  int i;
  
  gras_lock();

  netSize = DataSize(description, description_length, NETWORK_FORMAT);
  hostSize = DataSize(description, description_length, HOST_FORMAT);

  if (!(*data=malloc(hostSize*repetition))) {
      gras_unlock();
      ERROR1("grasDataRecv: memory allocation of %d bytes failed\n", hostSize*repetition);
      return 0;
  }

  convertIt = grasConversionRequired(description, description_length);
  
  if(convertIt) {
    if (!(converted = malloc(netSize*repetition))) {
      free(*data);
      gras_unlock();
      ERROR1("RecvData: memory allocation of %d bytes failed\n", netSize*repetition);
      return 0;
    }
    destination = converted;
  } else {
    destination = *data;
  }
  
  /* adaptive timeout */
  start = CurrentTime();
  
  recvResult = RecvBytes(sd->sock, destination, netSize*repetition,
			 GetTimeOut(RECV, Peer(sd->sock), netSize*repetition));
  /* we assume a failure is a timeout ... Shouldn't hurt
   * too much getting a bigger timeout anyway */
  SetTimeOut(RECV, sd->sock, CurrentTime()-start, netSize*repetition, !recvResult);

  fprintf(stderr,"RECV [seqLen=%d;netsize=%d;hostsize=%d] : (", 
	  repetition,netSize,hostSize);
  for (i=0; i<netSize * repetition; i++) {
    if (i) fputc('.',stderr);
    fprintf(stderr,"%02X",((unsigned char*)destination)[i]);
  }
  fprintf(stderr,") on %p.\n",destination);

  if (recvResult != 0) {
    if(convertIt) {
      for (i=0, net=(char*)converted, host=(char*)*data; 
	   i<repetition;
	   i++, net += netSize, host += hostSize) {
	ConvertData((void*)host, (void*)net, description, description_length, NETWORK_FORMAT);
      }
    }
    if(converted != NULL)
      free(converted);
  }
  if (!gras_unlock()) return 0;
  
  return recvResult;
}

gras_error_t grasDataSend(gras_sock_t *sd,
			 const void *data,
			 const DataDescriptor *description,
			 size_t description_length,
			 int repetition) {
  
  void *converted;
  char *net,*host; /* iterators */
  int sendResult,i;
  const void *source;
  size_t netSize = DataSize(description, description_length, NETWORK_FORMAT);
  size_t hostSize = DataSize(description, description_length, HOST_FORMAT);
  double start; /* for timeouts */

  gras_lock();
  converted = NULL;

  if(grasConversionRequired(description, description_length)) {
    converted = malloc(netSize * repetition);
    if(converted == NULL) {
      gras_unlock();
      fprintf(stderr,"grasDataSend: memory allocation of %d bytes failed.\n",netSize * repetition);
      return malloc_error;
    }
    
    for (i=0, net=(char*)converted, host=(char*)data;
	 i<repetition;
	 i++, net += netSize, host += hostSize)
      ConvertData((void*)net, (void*)host, description, description_length, HOST_FORMAT);
    source = converted;
  } else {
    source = data;
  }

  fprintf(stderr,"SEND (");
  for (i=0; i<netSize * repetition; i++) {
    if (i) fputc('.',stderr);
    fprintf(stderr,"%02X",((unsigned char*)source)[i]);
  }
  fprintf(stderr,") from %p\n",source);
  // grasDataDescDump((const DataDescriptor *)description, description_length);
    
  /* adaptive timeout */
  start = CurrentTime();
  sendResult = SendBytes(sd->sock, source, netSize * repetition,
			 GetTimeOut(SEND, Peer(sd->sock),netSize*repetition));
  /* we assume a failure is a timeout ... Shouldn't hurt
   * too much getting a bigger timeout anyway */
  SetTimeOut(SEND, sd->sock, CurrentTime()-start, netSize * repetition, !sendResult);
  if(converted != NULL)
    free((void *)converted);

  gras_unlock();
  return no_error;
}

gras_error_t
grasMsgRecv(gras_msg_t **msg,
	    double timeOut) {
  int dummyldap;
  gras_msg_t *res;
  size_t recvd; /* num of bytes received */
  int i;
  gras_sock_t *sd;
  
  if (!(sd = (gras_sock_t*)malloc(sizeof(gras_sock_t)))) {
    fprintf(stderr,"grasMsgRecv: Malloc error\n");
    return malloc_error;
  }
  
  if(!IncomingRequest(timeOut, &(sd->sock), &dummyldap)) {
    free(sd);
    return timeout_error;
  }

  if (!gras_lock()) {
    free (sd);
    return thread_error;
  }
  if (!(res = malloc(sizeof(gras_msg_t)))) {
    gras_unlock();
    *msg=NULL;
    free(sd);
    return malloc_error;
  }
  *msg=res;
  res->sock=sd;
  res->freeDirective=free_after_use;

  if (!gras_unlock()) return thread_error;

  if(!(recvd=grasDataRecv( sd,
			   (void **)&(res->header),
			   headerDescriptor,headerDescriptorCount,1))) {
    fprintf(stderr,"grasMsgRecv: no message received\n");
    return network_error;
  }
  res->header->dataSize -= recvd;

  fprintf(stderr,"Received header=ver:'%s' msg:%d size:%d seqCount:%d\n",
	  res->header->version,  res->header->message,
	  res->header->dataSize, res->header->seqCount);
  if (strncmp(res->header->version,GRASVERSION,strlen(GRASVERSION))) {
    /* The other side do not use the same version than us. Let's panic */
    char junk[2046];
    int junkint;
    while ((junkint=recv(sd->sock,junk,2046,0)) == 2046);
    fprintf(stderr,"PANIC: Got a message from a peer (%s:%d) using GRAS version '%10s' while this side use version '%s'.\n",
	    gras_sock_get_peer_name(sd),gras_sock_get_peer_port(sd),
	    res->header->version,GRASVERSION);
    return mismatch_error;    
  }
  if (!(res->entry=grasMsgEntryGet(res->header->message))) {
    /* This message is not registered on our side, discard it */
    gras_msg_discard(sd,res->header->dataSize);

    i= res->header->message;
    free(res->header);
    free(res);
    *msg=NULL;
    
    fprintf(stderr,"grasMsgRecv: unregistered message %d received from %s\n",
	    i,gras_sock_get_peer_name(sd));
    return mismatch_error;
  }
  
  if (!(recvd=grasDataRecv( sd,
			    (void **)&(res->dataCount),
			    countDescriptor,res->entry->seqCount,1))) {
    gras_msg_discard(sd,res->header->dataSize);
    i = res->header->message;
    free(res->header);
    free(res);
    *msg=NULL;
    fprintf(stderr, "grasMsgRecv: Error while getting elemCounts in message %d received from %s\n",
	  i,gras_sock_get_peer_name(sd));
    return network_error;
  }
  res->header->dataSize -= recvd;

  if (!gras_lock()) return thread_error;
  if (!(res->data=(void**)malloc(sizeof(void*)*res->entry->seqCount))) {
    gras_msg_discard(sd,res->header->dataSize);

    i= res->header->message;
    free(res->header);
    free(res->dataCount);
    free(res);
    *msg=NULL;
    
    gras_unlock();
    fprintf(stderr,"grasMsgRecv: Out of memory while receiving message %d received from %s\n",
	  i,gras_sock_get_peer_name(sd));
    return malloc_error;
  }
  if (!gras_unlock()) return thread_error;

  for (i=0;i<res->entry->seqCount;i++) {

    if(!(recvd=grasDataRecv( sd,
			     &(res->data[i]),
			     (const DataDescriptor*)res->entry->dd[i],
			     res->entry->ddCount[i],
			     res->dataCount[i]) )) {
      gras_msg_discard(sd,res->header->dataSize);
      for (i--;i>=0;i--) free(res->data[i]);
      free(res->dataCount);
      free(res->data);
      free(res);
      *msg=NULL;
      fprintf(stderr,"grasDataRecv: Transmision error while receiving message %d received from %s\n",
	    i,gras_sock_get_peer_name(sd));
      return network_error;
    }
    res->header->dataSize -= recvd;
  }

  if (res->header->dataSize) {
    fprintf(stderr,"Damnit dataSize = %d != 0 after receiving message %d received from %s\n",
	    res->header->dataSize,i,gras_sock_get_peer_name(sd)); 
    return unknown_error;
  }
  return no_error;
}

/*
 * Send a message to the network
 */

gras_error_t
gras_msg_send(gras_sock_t *sd,
	    gras_msg_t *msg,
	    e_gras_free_directive_t freeDirective) {

  gras_error_t errcode;
  int i;

  /* arg validity checks */
  gras_assert0(msg,"Trying to send NULL message");
  gras_assert0(sd, "Trying to send over a NULL socket");


  fprintf(stderr,"Header to send=ver:'%s' msg:%d size:%d seqCount:%d\n",
	  msg->header->version,  msg->header->message,
	  msg->header->dataSize, msg->header->seqCount);

  /* Send the message */
  if ((errcode=grasDataSend(sd,
			    msg->header,
			    headerDescriptor,headerDescriptorCount,1))) {
    fprintf(stderr,"gras_msg_send: Error '%s' while sending header of message %s to %s:%d\n",
	    gras_error_name(errcode),msg->entry->name,gras_sock_get_peer_name(sd),gras_sock_get_peer_port(sd));
    return errcode;
  }

  if ((errcode=grasDataSend(sd,
			    msg->dataCount,countDescriptor,countDescriptorCount,
			    msg->entry->seqCount))) {
    fprintf(stderr,"gras_msg_send: Error '%s' while sending sequence counts of message %s to %s\n",
	    gras_error_name(errcode),msg->entry->name,gras_sock_get_peer_name(sd));
    return errcode;
  }

  for (i=0; i<msg->entry->seqCount; i++) {
    if ((errcode=grasDataSend(sd,
			      msg->data[i],
			      (const DataDescriptor*)msg->entry->dd[i],msg->entry->ddCount[i],
			      msg->dataCount[i]))) {
      fprintf(stderr,"gras_msg_send: Error '%s' while sending sequence %d of message %s to %s\n",
	      gras_error_name(errcode),i,msg->entry->name,gras_sock_get_peer_name(sd));
      return errcode;
    }
  }  
  
  if (freeDirective == free_after_use) gras_msg_free(msg);
  return no_error;
}

gras_sock_t *gras_sock_new(void) {
  return malloc(sizeof(gras_sock_t));
}

void grasSockFree(gras_sock_t *s) {
  if (s) free (s);
}

/* **************************************************************************
 * Creating/Using raw sockets
 * **************************************************************************/
gras_error_t gras_rawsock_server_open(unsigned short startingPort, 
				  unsigned short endingPort,
				  unsigned int bufSize, gras_rawsock_t **sock) {
  struct sockaddr_in sockaddr;


  if (!(*sock=malloc(sizeof(gras_rawsock_t)))) {
    fprintf(stderr,"Malloc error\n");
    return malloc_error;
  }

  
  for((*sock)->port = startingPort; 
      (*sock)->port <= endingPort; 
      (*sock)->port++) {
    if(((*sock)->sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      ERROR0("gras_rawsock_server_open: socket creation failed\n");
      free(*sock);
      return system_error;
    }

    setsockopt((*sock)->sock, SOL_SOCKET, SO_RCVBUF, (char *)&bufSize, sizeof(bufSize));
    setsockopt((*sock)->sock, SOL_SOCKET, SO_SNDBUF, (char *)&bufSize, sizeof(bufSize));

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_port = htons((unsigned short)(*sock)->port);
    sockaddr.sin_addr.s_addr = INADDR_ANY;
    sockaddr.sin_family = AF_INET;

    if (bind((*sock)->sock, 
             (struct sockaddr *)&sockaddr, sizeof(sockaddr)) != -1  &&
        listen((*sock)->sock, 1) != -1) {
      break;
    }
    close((*sock)->sock);
  }
  
  if((*sock)->port > endingPort) {
    fprintf(stderr,"gras_rawsock_server_open: couldn't find a port between %d and %d\n", 
	    startingPort, endingPort);
    free(*sock);
    return mismatch_error;
  }

  return no_error;
}

void Dummy(int);
void Dummy(int sig) {
  return;
}

gras_error_t gras_rawsock_client_open(const char *host, short port, 
				  unsigned int bufSize, gras_rawsock_t **sock) {
  int i,addrCount;
  IPAddress addresses[10];
  void (*was)(int);
  struct sockaddr_in sockaddr;

  if (!(*sock=malloc(sizeof(gras_rawsock_t)))) {
    fprintf(stderr,"Malloc error\n");
    return malloc_error;
  }
  (*sock)->port=-1;

  if (!(addrCount = IPAddressValues(host, addresses, 10))) {
    fprintf(stderr,"grasOpenClientSocket: address retrieval of '%s' failed\n",host);
    free(*sock);
    return system_error;
  }
  (*sock)->port = port;
  memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sin_port = htons((unsigned short)(*sock)->port);
  sockaddr.sin_family = AF_INET;

  for(i = 0; i < addrCount && i<10 ; i++) {
    if(((*sock)->sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      fprintf(stderr,"gras_rawsock_client_open: socket creation failed\n");
      free(*sock);
      return system_error;
    }
    
    setsockopt((*sock)->sock, SOL_SOCKET, SO_RCVBUF, 
	       (char *)&bufSize, sizeof(bufSize));
    setsockopt((*sock)->sock, SOL_SOCKET, SO_SNDBUF, 
	       (char *)&bufSize, sizeof(bufSize));

    sockaddr.sin_addr.s_addr = addresses[i];

    was = signal(SIGALRM,Dummy);
    SetRealTimer(GetTimeOut(CONN, addresses[i], 0));
    if(connect((*sock)->sock, 
	       (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == 0) {
      SetRealTimer(0);
      signal(SIGALRM,was);
      break;

    }
    SetRealTimer(0);
    close((*sock)->sock);
    signal(SIGALRM,was);
  }

  if (i>=10) {
    free(*sock);
    fprintf(stderr,
	    "grasOpenClientRawSocket: Unable to contact %s:%d\n",
	    host,port);
    return network_error;
  }
  
  return no_error;
}

gras_error_t gras_rawsock_close(gras_rawsock_t *sd) {
  if (sd) {
    CloseSocket(&(sd->sock), 0);
    free(sd);
  }
  return no_error;
}

unsigned short gras_rawsock_get_peer_port(gras_rawsock_t *sd) {
  if (!sd) return -1;
  return sd->port;
}

/* FIXME: RL ignores the provided timeout and compute an appropriate one */
static gras_error_t
_gras_rawsock_exchange(gras_rawsock_t *sd, int sender, unsigned int timeout,
		     unsigned int expSize, unsigned int msgSize){
  char *expData;
  int bytesThisCall;
  int bytesThisMessage;
  int bytesTotal;

  fd_set rd_set;
  int rv;

  char *name;
  IPAddress addr;

  int ltimeout;
  struct timeval timeOut;

  if((expData = (char *)malloc(msgSize)) == NULL) {
    fprintf(stderr,"gras_rawsock_send: malloc %d failed\n", msgSize);
    return malloc_error;
  }

  /* let's get information on the caller (needed later on) */
  name = PeerName_r(sd->sock);
  IPAddressValue(name, &addr);
  free(name);

  ltimeout = (int)GetTimeOut((sender ? SEND : RECV), addr, expSize/msgSize +1);

  if (sender)
    SetRealTimer(ltimeout);


  for(bytesTotal = 0;
      bytesTotal < expSize;
      bytesTotal += bytesThisMessage) {
    for(bytesThisMessage = 0;
        bytesThisMessage < msgSize;
        bytesThisMessage += bytesThisCall) {


      if(sender) {
	bytesThisCall = send(sd->sock, expData, msgSize - bytesThisMessage, 0);
      } else {
	bytesThisCall = 0;
	FD_ZERO(&rd_set);
	FD_SET(sd->sock,&rd_set);
	timeOut.tv_sec = ltimeout;
	timeOut.tv_usec = 0;
	/*
	 * YUK!  The timeout can get to be REALLY large if the
	 * amount of data gets big -- fence it at 60 secs
	 */
	if ((ltimeout <= 0) || (ltimeout > 60)) {
	  ltimeout = 60;
	}
	rv = select(sd->sock+1,&rd_set,NULL,NULL,&timeOut);
	if(rv > 0) {
	  bytesThisCall = recv(sd->sock, expData, msgSize-bytesThisMessage, 0);
	}
      }
    }
  }
  free(expData);
  return no_error;
}

gras_error_t
gras_rawsock_recv(gras_rawsock_t *sd, unsigned int expSize, unsigned int msgSize, 
		unsigned int timeout) {
  return _gras_rawsock_exchange(sd,0,timeout,expSize,msgSize);
}
gras_error_t
gras_rawsock_send(gras_rawsock_t *sd, unsigned int expSize, unsigned int msgSize){
  return _gras_rawsock_exchange(sd,1,0,expSize,msgSize);
}




/* **************************************************************************
 * Process data
 * **************************************************************************/
grasProcessData_t *grasProcessDataGet() {
  return _grasProcessData;
}

/* **************************************************************************
 * Wrappers over OS functions
 * **************************************************************************/
double gras_time() {
  return MicroTime();
}

void gras_sleep(unsigned long sec,unsigned long usec) {
  sleep(sec);
  if (usec/1000000) sleep(usec/1000000);
  (void)usleep(usec % 1000000);
}
