/* $Id$ */

/* gras_sg - legacy implementation of GRAS on top of the SimGrid simulator  */
/* This file should be KILLED whenever the raw sockets work in the new gras */

/* Copyright (c) 2003 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <string.h>

#include "gras_sg.h"

XBT_LOG_DEFAULT_CATEGORY(GRAS);

/* **************************************************************************
 * Openning/Maintaining/Closing connexions (private functions for both raw
 * and regular sockets)
 * **************************************************************************/
xbt_error_t
_gras_sock_server_open(unsigned short startingPort, unsigned short endingPort,
		      int raw, unsigned int bufSize, /* OUT */ gras_sock_t **sock) {

  gras_hostdata_t *hd=hd=(gras_hostdata_t *)MSG_host_get_data(MSG_host_self());
  grasProcessData_t *pd=(grasProcessData_t *)MSG_process_get_data(MSG_process_self());
  int port,i;
  const char *host=MSG_host_get_name(MSG_host_self());

  xbt_assert0(hd,"HostData=NULL !! Please run grasInit on each process\n");
  xbt_assert0(pd,"ProcessData=NULL !! Please run grasInit on each process\n");

  for (port=startingPort ; port <= endingPort ; port++) {
    for (i=0; i<hd->portLen && hd->port[i] != port; i++);
    if (i<hd->portLen && hd->port[i] == port)
      continue;

    /* port not used so far. Do it */
    if (i == hd->portLen) {
      /* need to enlarge the tables */
      if (hd->portLen++) {
	hd->port2chan=(int*)realloc(hd->port2chan,hd->portLen*sizeof(int));
	hd->port     =(int*)realloc(hd->port     ,hd->portLen*sizeof(int));
	hd->raw      =(int*)realloc(hd->raw      ,hd->portLen*sizeof(int));
      } else {
	hd->port2chan=(int*)malloc(hd->portLen*sizeof(int));
	hd->port     =(int*)malloc(hd->portLen*sizeof(int));
	hd->raw      =(int*)malloc(hd->portLen*sizeof(int));
      }
      if (!hd->port2chan || !hd->port || !hd->raw) {
	fprintf(stderr,"GRAS: PANIC: A malloc error did lose all ports attribution on this host\n");
	hd->portLen = 0;
	return malloc_error;
      }
    }
    hd->port2chan[ i ]=raw ? pd->rawChan : pd->chan;
    hd->port[ i ]=port;
    hd->raw[ i ]=raw;

    /* Create the socket */
    if (!(*sock=(gras_sock_t*)malloc(sizeof(gras_sock_t)))) {
      fprintf(stderr,"GRAS: openServerSocket: out of memory\n");
      return malloc_error;
    }    
    
    (*sock)->server_sock  = 1;
    (*sock)->raw_sock     = raw;
    (*sock)->from_PID     = -1;
    (*sock)->to_PID       = MSG_process_self_PID();
    (*sock)->to_host      = MSG_host_self();
    (*sock)->to_port      = port;  
    (*sock)->to_chan      = pd->chan;

    /*
    fprintf(stderr,"GRAS: '%s' (%d) ears on %s:%d%s (%p).\n",
	    MSG_process_get_name(MSG_process_self()), MSG_process_self_PID(),
	    host,port,raw? " (mode RAW)":"",*sock);
    */
    return no_error;
  }
  /* if we go out of the previous for loop, that's that we didn't find any
     suited port number */

  fprintf(stderr,
	  "GRAS: can't find an empty port between %d and %d to open a socket on host %s\n.",
	  startingPort,endingPort,host);
  return mismatch_error;
}

xbt_error_t
_gras_sock_client_open(const char *host, short port, int raw, unsigned int bufSize,
		     /* OUT */ gras_sock_t **sock) {
  m_host_t peer;
  gras_hostdata_t *hd;
  int i;

  /* make sure this socket will reach someone */
  if (!(peer=MSG_get_host_by_name(host))) {
      fprintf(stderr,"GRAS: can't connect to %s: no such host.\n",host);
      return mismatch_error;
  }
  if (!(hd=(gras_hostdata_t *)MSG_host_get_data(peer))) {
      fprintf(stderr,"GRAS: can't connect to %s: no process on this host.\n",host);
      return mismatch_error;
  }
  for (i=0; i<hd->portLen && port != hd->port[i]; i++);
  if (i == hd->portLen) {
    fprintf(stderr,"GRAS: can't connect to %s:%d, no process listen on this port.\n",host,port);
    return mismatch_error;
  } 

  if (hd->raw[i] && !raw) {
    fprintf(stderr,"GRAS: can't connect to %s:%d in regular mode, the process listen in raw mode on this port.\n",host,port);
    return mismatch_error;
  }
  if (!hd->raw[i] && raw) {
    fprintf(stderr,"GRAS: can't connect to %s:%d in raw mode, the process listen in regular mode on this port.\n",host,port);
    return mismatch_error;
  }
    

  /* Create the socket */
  if (!(*sock=(gras_sock_t*)malloc(sizeof(gras_sock_t)))) {
      fprintf(stderr,"GRAS: openClientSocket: out of memory\n");
      return malloc_error;
  }    

  (*sock)->server_sock  = 0;
  (*sock)->raw_sock     = raw;
  (*sock)->from_PID     = MSG_process_self_PID();
  (*sock)->to_PID       = hd->proc[ hd->port2chan[i] ];
  (*sock)->to_host      = peer;
  (*sock)->to_port      = port;  
  (*sock)->to_chan      = hd->port2chan[i];

  /*
  fprintf(stderr,"GRAS: %s (PID %d) connects in %s mode to %s:%d (to_PID=%d).\n",
	  MSG_process_get_name(MSG_process_self()), MSG_process_self_PID(),
	  raw?"RAW":"regular",host,port,(*sock)->to_PID);
  */
  return no_error;
}

xbt_error_t _gras_sock_close(int raw, gras_sock_t *sd) {
  gras_hostdata_t *hd=(gras_hostdata_t *)MSG_host_get_data(MSG_host_self());
  int i;

  xbt_assert0(hd,"HostData=NULL !! Please run grasInit on each process\n");

  if (!sd) return no_error;
  if (raw && !sd->raw_sock) {
      fprintf(stderr,"GRAS: gras_rawsock_close: Was passed a regular socket. Please use gras_sock_close()\n");
  }
  if (!raw && sd->raw_sock) {
      fprintf(stderr,"GRAS: grasSockClose: Was passed a raw socket. Please use gras_rawsock_close()\n");
  }
  if (sd->server_sock) {
    /* server mode socket. Un register it from 'OS' tables */
    for (i=0; 
	 i<hd->portLen && sd->to_port != hd->port[i]; 
	 i++);

    if (i==hd->portLen) {
      fprintf(stderr,"GRAS: closeSocket: The host does not know this server socket.\n");
    } else {
      memmove(&(hd->port[i]),      &(hd->port[i+1]),      (hd->portLen -i -1) * sizeof(int));
      memmove(&(hd->raw[i]),       &(hd->raw[i+1]),       (hd->portLen -i -1) * sizeof(int));
      memmove(&(hd->port2chan[i]), &(hd->port2chan[i+1]), (hd->portLen -i -1) * sizeof(int));
      hd->portLen--;
    }
  } 
  free(sd);
  return no_error;
}

/* **************************************************************************
 * Creating/Using regular sockets
 * **************************************************************************/
xbt_error_t
gras_sock_server_open(unsigned short startingPort, unsigned short endingPort,
		     /* OUT */ gras_sock_t **sock) {
  return _gras_sock_server_open(startingPort,endingPort,0,0,sock);
}

xbt_error_t
gras_sock_client_open(const char *host, short port,
		     /* OUT */ gras_sock_t **sock) {
  return _gras_sock_client_open(host,port,0,0,sock);
}

xbt_error_t gras_sock_close(gras_sock_t *sd) {
  return _gras_sock_close(0,sd);
}

unsigned short
gras_sock_get_my_port(gras_sock_t *sd) {
  if (!sd || !sd->server_sock) return -1;
  return sd->to_port;
}

unsigned short
gras_sock_get_peer_port(gras_sock_t *sd) {
  if (!sd || sd->server_sock) return -1;
  return sd->to_port;
}

char *
gras_sock_get_peer_name(gras_sock_t *sd) {
  m_process_t proc;

  if (!sd) return NULL;
  if ((proc=MSG_process_from_PID(sd->to_PID))) {
    return (char*) MSG_host_get_name(MSG_process_get_host(proc));
  } else {
    fprintf(stderr,"GRAS: try to access hostname of unknown process %d\n",sd->to_PID);
    return (char*) "(dead or unknown host)";
  }
}
/* **************************************************************************
 * Creating/Using raw sockets
 * **************************************************************************/
xbt_error_t gras_rawsock_server_open(unsigned short startingPort, unsigned short endingPort,
				  unsigned int bufSize, gras_rawsock_t **sock) {
  return _gras_sock_server_open(startingPort,endingPort,1,bufSize,(gras_sock_t**)sock);
}

xbt_error_t gras_rawsock_client_open(const char *host, short port, 
				  unsigned int bufSize, gras_rawsock_t **sock) {
  return _gras_sock_client_open(host,port,1,bufSize,(gras_sock_t**)sock);
}

xbt_error_t gras_rawsock_close(gras_rawsock_t *sd) {
  return _gras_sock_close(1,(gras_sock_t*)sd);
}

unsigned short
gras_rawsock_get_peer_port(gras_rawsock_t *sd) {
  if (!sd || !sd->server_sock) return -1;
  return sd->to_port;
}

xbt_error_t
gras_rawsock_send(gras_rawsock_t *sock, unsigned int expSize, unsigned int msgSize) {
  unsigned int bytesTotal;
  static unsigned int count=0;
  m_task_t task=NULL;
  char name[256];

  xbt_assert0(sock->raw_sock,"Asked to send raw data on a regular socket\n");

  for(bytesTotal = 0;
      bytesTotal < expSize;
      bytesTotal += msgSize) {
    
    sprintf(name,"Raw data[%d]",count++);

    task=MSG_task_create(name,0,((double)msgSize)/(1024.0*1024.0),NULL);
    /*
    fprintf(stderr, "%f:%s: gras_rawsock_send(%f %s -> %s) BEGIN\n",
	    gras_time(),
	    MSG_process_get_name(MSG_process_self()),
	    ((double)msgSize)/(1024.0*1024.0),
	    MSG_host_get_name( MSG_host_self()),
	    MSG_host_get_name( sock->to_host));
    */
    if (MSG_task_put(task, sock->to_host,sock->to_chan) != MSG_OK) {
      fprintf(stderr,"GRAS: msgSend: Problem during the MSG_task_put()\n");
      return unknown_error;
    }
    /*fprintf(stderr, "%f:%s: gras_rawsock_send(%f -> %s) END\n",
	    gras_time(),
	    MSG_process_get_name(MSG_process_self()),
	    ((double)msgSize)/(1024.0*1024.0),
	    MSG_host_get_name( sock->to_host));*/
  }
  return no_error;
}

xbt_error_t
gras_rawsock_recv(gras_rawsock_t *sock, unsigned int expSize, unsigned int msgSize,
		unsigned int timeout) {
  grasProcessData_t *pd=(grasProcessData_t *)MSG_process_get_data(MSG_process_self());
  unsigned int bytesTotal;
  m_task_t task=NULL;
  double startTime;

  xbt_assert0(sock->raw_sock,"Asked to receive raw data on a regular socket\n");

  startTime=gras_time();
  for(bytesTotal = 0;
      bytesTotal < expSize;
      bytesTotal += msgSize) {
    
    task=NULL;
    /*
    fprintf(stderr, "%f:%s: gras_rawsock_recv() BEGIN\n",
	    gras_time(),
	    MSG_process_get_name(MSG_process_self()));
    */
    do {
      if (MSG_task_Iprobe((m_channel_t) pd->rawChan)) { 
	if (MSG_task_get(&task, (m_channel_t) pd->rawChan) != MSG_OK) {
	  fprintf(stderr,"GRAS: Error in MSG_task_get()\n");
	  return unknown_error;
	}
	if (MSG_task_destroy(task) != MSG_OK) {
	  fprintf(stderr,"GRAS: Error in MSG_task_destroy()\n");
	  return unknown_error;
	}
	/*
	fprintf(stderr, "%f:%s: gras_rawsock_recv() END\n",
		gras_time(),
		MSG_process_get_name(MSG_process_self()));
	*/
	break;
      } else { 
	MSG_process_sleep(0.0001);
      }
    } while (gras_time() - startTime < timeout);

    if (gras_time() - startTime > timeout)
      return timeout_error;
  }
  return no_error;
}


/* **************************************************************************
 * Actually exchanging messages
 * **************************************************************************/

xbt_error_t
grasMsgRecv(gras_msg_t **msg,
	    double timeOut) {

  double startTime=gras_time();
  grasProcessData_t *pd=grasProcessDataGet();
  m_task_t task=NULL;

  do {
    if (MSG_task_Iprobe((m_channel_t) pd->chan)) {
      if (MSG_task_get(&task, (m_channel_t) pd->chan) != MSG_OK) {
	fprintf(stderr,"GRAS: Error in MSG_task_get()\n");
	return unknown_error;
      }
      
      *msg=(gras_msg_t*)MSG_task_get_data(task);
      /*
	{ 
	int i,j;
	gras_msg_t *__dm_=*msg;
	
	fprintf(stderr, "grasMsgRecv(%s) = %d seq (",
	__dm_->entry->name, __dm_->entry->seqCount );
	
	for (i=0; i<__dm_->entry->seqCount; i++) {
	fprintf(stderr,"%d elem {",__dm_->dataCount[i]);
	for (j=0; j<__dm_->dataCount[i]; j++) { 
	fprintf(stderr,"%p; ",__dm_->data[i]);
	}
	fprintf(stderr,"},");
	}
	fprintf(stderr, ")\n");
	}
      */

      if (MSG_task_destroy(task) != MSG_OK) {
	fprintf(stderr,"GRAS: Error in MSG_task_destroy()\n");
	return unknown_error;
      }
      return no_error;

    } else {
      MSG_process_sleep(1);
    }
  } while (gras_time()-startTime < timeOut || MSG_task_Iprobe((m_channel_t) pd->chan));
  return timeout_error;
}

xbt_error_t
gras_msg_send(gras_sock_t *sd,
	    gras_msg_t *_msg,
	    e_xbt_free_directive_t freeDirective) {
  
  grasProcessData_t *pd=grasProcessDataGet();
  m_task_t task;
  static int count=0;
  char name[256];
  gras_msg_t *msg;
  gras_sock_t *answer;

  /* arg validity checks */
  xbt_assert0(msg,"Trying to send NULL message");
  xbt_assert0(sd ,"Trying to send over a NULL socket");
  
  if (!(answer=(gras_sock_t*)malloc(sizeof(gras_sock_t)))) {
    RAISE_MALLOC;
  }
  answer->server_sock=0;
  answer->raw_sock   =0;
  answer->from_PID   = sd->to_PID;
  answer->to_PID     = MSG_process_self_PID();
  answer->to_host    = MSG_host_self();
  answer->to_port    = 0;
  answer->to_chan    = pd->chan;
    
  sprintf(name,"%s[%d]",_msg->entry->name,count++);
  /* if freeDirective == free_never, we have to build a copy of the message */
  if (freeDirective == free_never) {
    msg=gras_msg_copy(_msg);
  } else {
    msg=_msg;
  }
  msg->sock = answer;

  /*
  fprintf(stderr,"Send %s with answer(%p)=",msg->entry->name,msg->sock);
  fprintf(stderr,"(server=%d,raw=%d,fromPID=%d,toPID=%d,toHost=%p,toPort=%d,toChan=%d)\n",
	  msg->sock->server_sock,msg->sock->raw_sock,msg->sock->from_PID,
	  msg->sock->to_PID,msg->sock->to_host,msg->sock->to_port,msg->sock->to_chan);
  fprintf(stderr,"Send over %p=(server=%d,raw=%d,fromPID=%d,toPID=%d,toHost=%p,toPort=%d,toChan=%d)\n",
	  sd,sd->server_sock,sd->raw_sock,sd->from_PID,sd->to_PID,sd->to_host,sd->to_port,sd->to_chan);
  */

  /*
  { 
    int i,j;
    gras_msg_t *__dm_=msg;

    fprintf(stderr, "gras_msg_send(%s) = %d seq (",
	    __dm_->entry->name, __dm_->entry->seqCount );

    for (i=0; i<__dm_->entry->seqCount; i++) {
      fprintf(stderr,"%d elem {",__dm_->dataCount[i]);
      for (j=0; j<__dm_->dataCount[i]; j++) { 
	fprintf(stderr,"%p; ",__dm_->data[i]);
      }
      fprintf(stderr,"},");
    }
    fprintf(stderr, ")\n");
  }
  */

  /* Send it */
  task=MSG_task_create(name,0,((double)msg->header->dataSize)/(1024.0*1024.0),msg);
  if (MSG_task_put(task, sd->to_host,sd->to_chan) != MSG_OK) {
    fprintf(stderr,"GRAS: msgSend: Problem during the MSG_task_put()\n");
    return unknown_error;
  }
  return no_error;
}

gras_sock_t *gras_sock_new(void) {
  return malloc(sizeof(gras_sock_t));
}

void grasSockFree(gras_sock_t *s) {
  if (s) free (s);
}

/* **************************************************************************
 * Process data
 * **************************************************************************/
grasProcessData_t *grasProcessDataGet() {
  return (grasProcessData_t *)MSG_process_get_data(MSG_process_self());
}

/* **************************************************************************
 * Wrappers over OS functions
 * **************************************************************************/
double gras_time() {
  return MSG_getClock();
}

void gras_sleep(unsigned long sec, unsigned long usec) {
  MSG_process_sleep((double)sec + ((double)usec)/1000000);
}

