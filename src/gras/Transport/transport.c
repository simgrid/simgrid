/* $Id$ */

/* transport - low level communication                                      */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "portable.h"
#include "gras/Transport/transport_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(transport,gras,"Conveying bytes over the network");
XBT_LOG_NEW_SUBCATEGORY(meas_trp,transport,"Conveying bytes over the network without formating for perf measurements");
static short int _gras_trp_started = 0;

static xbt_dict_t _gras_trp_plugins;      /* All registered plugins */
static void gras_trp_plugin_free(void *p); /* free one of the plugins */

static void
gras_trp_plugin_new(const char *name, gras_trp_setup_t setup) {
  xbt_error_t errcode;

  gras_trp_plugin_t *plug = xbt_new0(gras_trp_plugin_t, 1);
  
  DEBUG1("Create plugin %s",name);

  plug->name=xbt_strdup(name);

  errcode = setup(plug);
  switch (errcode) {
  case mismatch_error:
    /* SG plugin return mismatch when in RL mode (and vice versa) */
    free(plug->name);
    free(plug);
    break;

  case no_error:
    xbt_dict_set(_gras_trp_plugins,
		  name, plug, gras_trp_plugin_free);
    break;

  default:
    DIE_IMPOSSIBLE;
  }

}

void gras_trp_init(void){
  if (!_gras_trp_started) {
     /* make room for all plugins */
     _gras_trp_plugins=xbt_dict_new();

#ifdef HAVE_WINSOCK2_H
     /* initialize the windows mechanism */
     {  
	WORD wVersionRequested;
	WSADATA wsaData;
	
	wVersionRequested = MAKEWORD( 2, 0 );
	xbt_assert0(WSAStartup( wVersionRequested, &wsaData ) == 0,
		    "Cannot find a usable WinSock DLL");
	
	/* Confirm that the WinSock DLL supports 2.0.*/
	/* Note that if the DLL supports versions greater    */
	/* than 2.0 in addition to 2.0, it will still return */
	/* 2.0 in wVersion since that is the version we      */
	/* requested.                                        */
	
	xbt_assert0(LOBYTE( wsaData.wVersion ) == 2 &&
		    HIBYTE( wsaData.wVersion ) == 0,
		    "Cannot find a usable WinSock DLL");
	INFO0("Found and initialized winsock2");
     }       /* The WinSock DLL is acceptable. Proceed. */
#elif HAVE_WINSOCK_H
     {       WSADATA wsaData;
	xbt_assert0(WSAStartup( 0x0101, &wsaData ) == 0,
		    "Cannot find a usable WinSock DLL");
	INFO0("Found and initialized winsock");
     }
#endif
   
     /* Add plugins */
     gras_trp_plugin_new("tcp", gras_trp_tcp_setup);
     gras_trp_plugin_new("file",gras_trp_file_setup);
     gras_trp_plugin_new("sg",gras_trp_sg_setup);

     /* buf is composed, so it must come after the others */
     gras_trp_plugin_new("buf", gras_trp_buf_setup);
  }
   
  _gras_trp_started++;
}

void
gras_trp_exit(void){
   if (_gras_trp_started == 0) {
      return;
   }
   
   if ( --_gras_trp_started == 0 ) {
#ifdef HAVE_WINSOCK_H
      if ( WSACleanup() == SOCKET_ERROR ) {
	 if ( WSAGetLastError() == WSAEINPROGRESS ) {
	    WSACancelBlockingCall();
	    WSACleanup();
	 }
	}
#endif

      xbt_dict_free(&_gras_trp_plugins);
   }
}


void gras_trp_plugin_free(void *p) {
  gras_trp_plugin_t *plug = p;

  if (plug) {
    if (plug->exit) {
      plug->exit(plug);
    } else if (plug->data) {
      DEBUG1("Plugin %s lacks exit(). Free data anyway.",plug->name);
      free(plug->data);
    }

    free(plug->name);
    free(plug);
  }
}


/**
 * gras_trp_socket_new:
 *
 * Malloc a new socket, and initialize it with defaults
 */
void gras_trp_socket_new(int incoming,
			 gras_socket_t *dst) {

  gras_socket_t sock=xbt_new0(s_gras_socket_t,1);

  DEBUG1("Create a new socket (%p)", (void*)sock);

  sock->plugin = NULL;
  sock->sd     = -1;
  sock->data   = NULL;

  sock->incoming  = incoming ? 1:0;
  sock->outgoing  = incoming ? 0:1;
  sock->accepting = incoming ? 1:0;

  sock->port      = -1;
  sock->peer_port = -1;
  sock->peer_name = NULL;
  sock->meas = 0;

  *dst = sock;

  xbt_dynar_push(gras_socketset_get(),dst);
  XBT_OUT;
}


/**
 * gras_socket_server_ext:
 *
 * Opens a server socket and make it ready to be listened to.
 * In real life, you'll get a TCP socket.
 */
xbt_error_t
gras_socket_server_ext(unsigned short port,
		       
		       unsigned long int bufSize,
		       int measurement,
		       
		       /* OUT */ gras_socket_t *dst) {
 
  xbt_error_t errcode;
  gras_trp_plugin_t *trp;
  gras_socket_t sock;

  *dst = NULL;

  DEBUG2("Create a server socket from plugin %s on port %d",
	 gras_if_RL() ? "tcp" : "sg",
	 port);
  TRY(gras_trp_plugin_get_by_name("buf",&trp));

  /* defaults settings */
  gras_trp_socket_new(1,&sock);
  sock->plugin= trp;
  sock->port=port;
  sock->bufSize = bufSize;
  sock->meas = measurement;

  /* Call plugin socket creation function */
  DEBUG1("Prepare socket with plugin (fct=%p)",trp->socket_server);
  errcode = trp->socket_server(trp, sock);
  DEBUG3("in=%c out=%c accept=%c",
	 sock->incoming?'y':'n', 
	 sock->outgoing?'y':'n',
	 sock->accepting?'y':'n');

  if (errcode != no_error) {
    free(sock);
    return errcode;
  }

  *dst = sock;

  return no_error;
}
   
/**
 * gras_socket_client_ext:
 *
 * Opens a client socket to a remote host.
 * In real life, you'll get a TCP socket.
 */
xbt_error_t
gras_socket_client_ext(const char *host,
		       unsigned short port,
		       
		       unsigned long int bufSize,
		       int meas,
		       
		       /* OUT */ gras_socket_t *dst) {
 
  xbt_error_t errcode;
  gras_trp_plugin_t *trp;
  gras_socket_t sock;

  *dst = NULL;

  TRY(gras_trp_plugin_get_by_name("buf",&trp));

  DEBUG1("Create a client socket from plugin %s",gras_if_RL() ? "tcp" : "sg");
  /* defaults settings */
  gras_trp_socket_new(0,&sock);
  sock->plugin= trp;
  sock->peer_port = port;
  sock->peer_name = (char*)strdup(host?host:"localhost");
  sock->bufSize = bufSize;
  sock->meas = meas;

  /* plugin-specific */
  errcode= (*trp->socket_client)(trp, sock);
  DEBUG3("in=%c out=%c accept=%c",
	 sock->incoming?'y':'n', 
	 sock->outgoing?'y':'n',
	 sock->accepting?'y':'n');

  if (errcode != no_error) {
    free(sock);
    return errcode;
  }

  *dst = sock;

  return no_error;
}

/**
 * gras_socket_server:
 *
 * Opens a server socket and make it ready to be listened to.
 * In real life, you'll get a TCP socket.
 */
xbt_error_t
gras_socket_server(unsigned short port,
		   /* OUT */ gras_socket_t *dst) {
   return gras_socket_server_ext(port,32,0,dst);
}

/**
 * gras_socket_client:
 *
 * Opens a client socket to a remote host.
 * In real life, you'll get a TCP socket.
 */
xbt_error_t
gras_socket_client(const char *host,
		   unsigned short port,
		   /* OUT */ gras_socket_t *dst) {
   return gras_socket_client_ext(host,port,32,0,dst);
}


void gras_socket_close(gras_socket_t sock) {
  xbt_dynar_t sockets = gras_socketset_get();
  gras_socket_t sock_iter;
  int cursor;

  XBT_IN;
  /* FIXME: Issue an event when the socket is closed */
  if (sock) {
    xbt_dynar_foreach(sockets,cursor,sock_iter) {
      if (sock == sock_iter) {
	xbt_dynar_cursor_rm(sockets,&cursor);
	if (sock->plugin->socket_close) 
	  (* sock->plugin->socket_close)(sock);

	/* free the memory */
	if (sock->peer_name)
	  free(sock->peer_name);
	free(sock);
	XBT_OUT;
	return;
      }
    }
    WARN0("Ignoring request to free an unknown socket");
  }
  XBT_OUT;
}

/**
 * gras_trp_chunk_send:
 *
 * Send a bunch of bytes from on socket
 */
xbt_error_t
gras_trp_chunk_send(gras_socket_t sd,
		    char *data,
		    long int size) {
  xbt_assert1(sd->outgoing,
	       "Socket not suited for data send (outgoing=%c)",
	       sd->outgoing?'y':'n');
  xbt_assert1(sd->plugin->chunk_send,
	       "No function chunk_send on transport plugin %s",
	       sd->plugin->name);
  return (*sd->plugin->chunk_send)(sd,data,size);
}
/**
 * gras_trp_chunk_recv:
 *
 * Receive a bunch of bytes from a socket
 */
xbt_error_t 
gras_trp_chunk_recv(gras_socket_t sd,
		    char *data,
		    long int size) {
  xbt_assert0(sd->incoming,
	       "Socket not suited for data receive");
  xbt_assert1(sd->plugin->chunk_recv,
	       "No function chunk_recv on transport plugin %s",
	       sd->plugin->name);
  return (sd->plugin->chunk_recv)(sd,data,size);
}

/**
 * gras_trp_flush:
 *
 * Make sure all pending communications are done
 */
xbt_error_t 
gras_trp_flush(gras_socket_t sd) {
  return (sd->plugin->flush)(sd);
}

xbt_error_t
gras_trp_plugin_get_by_name(const char *name,
			    gras_trp_plugin_t **dst){

  return xbt_dict_get(_gras_trp_plugins,name,(void**)dst);
}

int   gras_socket_my_port  (gras_socket_t sock) {
  return sock->port;
}
int   gras_socket_peer_port(gras_socket_t sock) {
  return sock->peer_port;
}
char *gras_socket_peer_name(gras_socket_t sock) {
  return sock->peer_name;
}

xbt_error_t gras_socket_meas_send(gras_socket_t peer, 
				  unsigned int timeout,
				  unsigned long int exp_size, 
				  unsigned long int msg_size) {
  xbt_error_t errcode;
  char *chunk = xbt_malloc(msg_size);
  int exp_sofar;
   
  XBT_IN;
  xbt_assert0(peer->meas,"Asked to send measurement data on a regular socket");
  for (exp_sofar=0; exp_sofar < exp_size; exp_sofar += msg_size) {
     CDEBUG5(meas_trp,"Sent %d of %lu (msg_size=%ld) to %s:%d",
	     exp_sofar,exp_size,msg_size,
	     gras_socket_peer_name(peer), gras_socket_peer_port(peer));
     TRY(gras_trp_chunk_send(peer,chunk,msg_size));
  }
  CDEBUG5(meas_trp,"Sent %d of %lu (msg_size=%ld) to %s:%d",
	  exp_sofar,exp_size,msg_size,
	  gras_socket_peer_name(peer), gras_socket_peer_port(peer));
	     
  free(chunk);
  XBT_OUT;
  return no_error;/* gras_socket_meas_exchange(peer,1,timeout,expSize,msgSize);    */
}

xbt_error_t gras_socket_meas_recv(gras_socket_t peer, 
				  unsigned int timeout,
				  unsigned long int exp_size, 
				  unsigned long int msg_size){
  
  xbt_error_t errcode;
  char *chunk = xbt_malloc(msg_size);
  int exp_sofar;

  XBT_IN;
  xbt_assert0(peer->meas,"Asked to receive measurement data on a regular socket\n");
  for (exp_sofar=0; exp_sofar < exp_size; exp_sofar += msg_size) {
     CDEBUG5(meas_trp,"Recvd %d of %lu (msg_size=%ld) from %s:%d",
	     exp_sofar,exp_size,msg_size,
	     gras_socket_peer_name(peer), gras_socket_peer_port(peer));
     TRY(gras_trp_chunk_recv(peer,chunk,msg_size));
  }
  CDEBUG5(meas_trp,"Recvd %d of %lu (msg_size=%ld) from %s:%d",
	  exp_sofar,exp_size,msg_size,
	  gras_socket_peer_name(peer), gras_socket_peer_port(peer));

  free(chunk);
  XBT_OUT;
  return no_error;/* gras_socket_meas_exchange(peer,0,timeout,expSize,msgSize);    */
}

/*
 * Creating procdata for this module
 */
static void *gras_trp_procdata_new() {
   gras_trp_procdata_t res = xbt_new(s_gras_trp_procdata_t,1);
   
   res->sockets   = xbt_dynar_new(sizeof(gras_socket_t*), NULL);
   
   return (void*)res;
}

/*
 * Freeing procdata for this module
 */
static void gras_trp_procdata_free(void *data) {
   gras_trp_procdata_t res = (gras_trp_procdata_t)data;
   
   xbt_dynar_free(&( res->sockets ));
}

/*
 * Module registration
 */
void gras_trp_register() {
   gras_procdata_add("gras_trp",gras_trp_procdata_new, gras_trp_procdata_free);
}


xbt_dynar_t 
gras_socketset_get(void) {
   /* FIXME: KILLME */
   return ((gras_trp_procdata_t) gras_libdata_get("gras_trp"))->sockets;
}
