/* $Id$ */

/* tcp trp (transport) - send/receive a bunch of bytes from a tcp socket    */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "portable.h"
#include "xbt/ex.h"
#if 0
#  include <signal.h>       /* close() pipe() read() write() */
#  include <sys/wait.h>     /* waitpid() */
#endif


#include "transport_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(trp_tcp,transport,"TCP transport");

/***
 *** Prototypes 
 ***/
void gras_trp_tcp_socket_client(gras_trp_plugin_t self, gras_socket_t sock);
void gras_trp_tcp_socket_server(gras_trp_plugin_t self, gras_socket_t sock);
gras_socket_t gras_trp_tcp_socket_accept(gras_socket_t  sock);

void          gras_trp_tcp_socket_close(gras_socket_t sd);
  
void gras_trp_tcp_chunk_send(gras_socket_t sd,
			     const char *data,
			     unsigned long int size);

void gras_trp_tcp_chunk_recv(gras_socket_t sd,
			     char *data,
			     unsigned long int size,
			     unsigned long int bufsize);

void gras_trp_tcp_exit(gras_trp_plugin_t plug);


static int TcpProtoNumber(void);
/***
 *** Specific plugin part
 ***/

typedef struct {
  fd_set msg_socks;
  fd_set meas_socks;
} gras_trp_tcp_plug_data_t;

/***
 *** Specific socket part
 ***/

typedef struct {
  int buffsize;
} gras_trp_tcp_sock_data_t;


/***
 *** Code
 ***/
void gras_trp_tcp_setup(gras_trp_plugin_t plug) {

  gras_trp_tcp_plug_data_t *data = xbt_new(gras_trp_tcp_plug_data_t,1);

  FD_ZERO(&(data->msg_socks));
  FD_ZERO(&(data->meas_socks));

  plug->socket_client = gras_trp_tcp_socket_client;
  plug->socket_server = gras_trp_tcp_socket_server;
  plug->socket_accept = gras_trp_tcp_socket_accept;
  plug->socket_close  = gras_trp_tcp_socket_close;

  plug->chunk_send    = gras_trp_tcp_chunk_send;
  plug->chunk_recv    = gras_trp_tcp_chunk_recv;

  plug->flush = NULL; /* nothing's cached */

  plug->data = (void*)data;
  plug->exit = gras_trp_tcp_exit;
}

void gras_trp_tcp_exit(gras_trp_plugin_t plug) {
  DEBUG1("Exit plugin TCP (free %p)", plug->data);
  free(plug->data);
}

void gras_trp_tcp_socket_client(gras_trp_plugin_t self, gras_socket_t sock){
  
  struct sockaddr_in addr;
  struct hostent *he;
  struct in_addr *haddr;
  int size = sock->bufSize * 1024; 

  sock->incoming = 1; /* TCP sockets are duplex'ed */

  sock->sd = socket (AF_INET, SOCK_STREAM, 0);
  
  if (sock->sd < 0) {
    THROW1(system_error,0, "Failed to create socket: %s", sock_errstr);
  }

  if (setsockopt(sock->sd, SOL_SOCKET, SO_RCVBUF, (char *)&size, sizeof(size)) ||
      setsockopt(sock->sd, SOL_SOCKET, SO_SNDBUF, (char *)&size, sizeof(size))) {
     WARN1("setsockopt failed, cannot set buffer size: %s",sock_errstr);
  }
  
  he = gethostbyname (sock->peer_name);
  if (he == NULL) {
    THROW2(system_error,0, "Failed to lookup hostname %s: %s",
	   sock->peer_name, sock_errstr);
  }
  
  haddr = ((struct in_addr *) (he->h_addr_list)[0]);
  
  memset(&addr, 0, sizeof(struct sockaddr_in));
  memcpy (&addr.sin_addr, haddr, sizeof(struct in_addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons (sock->peer_port);

  if (connect (sock->sd, (struct sockaddr*) &addr, sizeof (addr)) < 0) {
    tcp_close(sock->sd);
    THROW3(system_error,0,
	   "Failed to connect socket to %s:%d (%s)",
	   sock->peer_name, sock->peer_port, sock_errstr);
  }
  VERB4("Connect to %s:%d (sd=%d, port %d here)",sock->peer_name, sock->peer_port, sock->sd, sock->port);
}

/**
 * gras_trp_tcp_socket_server:
 *
 * Open a socket used to receive messages.
 */
void gras_trp_tcp_socket_server(gras_trp_plugin_t self, gras_socket_t sock){
  int size = sock->bufSize * 1024; 
  int on = 1;
  struct sockaddr_in server;

  gras_trp_tcp_plug_data_t *tcp=(gras_trp_tcp_plug_data_t*)self->data;
 
  sock->outgoing  = 1; /* TCP => duplex mode */

  server.sin_port = htons((u_short)sock->port);
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_family = AF_INET;
  if((sock->sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    THROW1(system_error,0,"Socket allocation failed: %s", sock_errstr);

  if (setsockopt(sock->sd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)))
     THROW1(system_error,0,"setsockopt failed, cannot condition the socket: %s",
	    sock_errstr);
   
  if (setsockopt(sock->sd, SOL_SOCKET, SO_RCVBUF, (char *)&size, sizeof(size)) ||
      setsockopt(sock->sd, SOL_SOCKET, SO_SNDBUF, (char *)&size, sizeof(size))) {
     WARN1("setsockopt failed, cannot set buffer size: %s",
	   sock_errstr);
  }
	
  if (bind(sock->sd, (struct sockaddr *)&server, sizeof(server)) == -1) {
    tcp_close(sock->sd);
    THROW2(system_error,0,"Cannot bind to port %d: %s",sock->port, sock_errstr);
  }

  DEBUG2("Listen on port %d (sd=%d)",sock->port, sock->sd);
  if (listen(sock->sd, 5) < 0) {
    tcp_close(sock->sd);
    THROW2(system_error,0,"Cannot listen on port %d: %s",sock->port,sock_errstr);
  }

  if (sock->meas)
    FD_SET(sock->sd, &(tcp->meas_socks));
  else
    FD_SET(sock->sd, &(tcp->msg_socks));

  VERB2("Openned a server socket on port %d (sd=%d)",sock->port,sock->sd);
}

gras_socket_t gras_trp_tcp_socket_accept(gras_socket_t  sock) {
  gras_socket_t res;
  
  struct sockaddr_in peer_in;
  socklen_t peer_in_len = sizeof(peer_in);

  int sd;
  int tmp_errno;
  int size;

  int i = 1;
  socklen_t s = sizeof(int);
		
  XBT_IN;
  gras_trp_socket_new(1,&res);

  sd = accept(sock->sd, (struct sockaddr *)&peer_in, &peer_in_len);
  tmp_errno = errno;

  if (sd == -1) {
    gras_socket_close(sock);
    THROW1(system_error,0,
	   "Accept failed (%s). Droping server socket.", sock_errstr);
  }
  
  if (setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, (char *)&i, s) 
      || setsockopt(sd, TcpProtoNumber(), TCP_NODELAY, (char *)&i, s))
    THROW1(system_error,0,"setsockopt failed, cannot condition the socket: %s",
	   sock_errstr);

  res->bufSize = sock->bufSize;
  size = sock->bufSize * 1024;
  if (setsockopt(sd, SOL_SOCKET, SO_RCVBUF, (char *)&size, sizeof(size))
      || setsockopt(sd, SOL_SOCKET, SO_SNDBUF, (char *)&size, sizeof(size)))
    WARN1("setsockopt failed, cannot set buffer size: %s", sock_errstr);
     
  res->plugin    = sock->plugin;
  res->incoming  = sock->incoming;
  res->outgoing  = sock->outgoing;
  res->accepting = 0;
  res->sd        = sd;
  res->port      = -1;
  res->peer_port = peer_in.sin_port;

  /* FIXME: Lock to protect inet_ntoa */
  if (((struct sockaddr *)&peer_in)->sa_family != AF_INET) {
    res->peer_name = (char*)strdup("unknown");
  } else {
    struct in_addr addrAsInAddr;
    char *tmp;
    
    addrAsInAddr.s_addr = peer_in.sin_addr.s_addr;
    
    tmp = inet_ntoa(addrAsInAddr);
    if (tmp != NULL) {
      res->peer_name = (char*)strdup(tmp);
    } else {
      res->peer_name = (char*)strdup("unknown");
    }
  }
  
  VERB3("Accepted from %s:%d (sd=%d)", res->peer_name,res->peer_port,sd);
  
  XBT_OUT;
  return res;
}

void gras_trp_tcp_socket_close(gras_socket_t sock){
  gras_trp_tcp_plug_data_t *tcp;
  
  if (!sock) return; /* close only once */
  tcp=sock->plugin->data;

  VERB1("close tcp connection %d", sock->sd);

  /* FIXME: no pipe in GRAS so far  
  if(!FD_ISSET(sd, &connectedPipes)) {
    if(shutdown(sd, 2) < 0) {
      GetNWSLock(&lock);
      tmp_errno = errno;
      ReleaseNWSLock(&lock);
      
      / * The other side may have beaten us to the reset. * /
      if ((tmp_errno!=ENOTCONN) && (tmp_errno!=ECONNRESET)) {
	WARN1("CloseSocket: shutdown error %d\n", tmp_errno);
      }
    }
  } */

#ifndef HAVE_WINSOCK_H
  /* forget about the socket 
     ... but not when using winsock since accept'ed socket can not fit 
     into the fd_set*/
  if (sock->meas){
    FD_CLR(sock->sd, &(tcp->meas_socks));
  } else {
    FD_CLR(sock->sd, &(tcp->msg_socks));
  }
#endif
   
  /* close the socket */
  if(tcp_close(sock->sd) < 0) {
    WARN3("error while closing tcp socket %d: %d (%s)\n", 
	     sock->sd, sock_errno, sock_errstr);
  }

}

/**
 * gras_trp_tcp_chunk_send:
 *
 * Send data on a TCP socket
 */
void
gras_trp_tcp_chunk_send(gras_socket_t sock,
			const char *data,
			unsigned long int size) {
  
  /* TCP sockets are in duplex mode, don't check direction */
  xbt_assert0(size >= 0, "Cannot send a negative amount of data");

  while (size) {
    int status = 0;
    
    status = tcp_write(sock->sd, data, (size_t)size);
    DEBUG3("write(%d, %p, %ld);", sock->sd, data, size);
    
    if (status < 0) {
      THROW4(system_error,0,"write(%d,%p,%ld) failed: %s",
	     sock->sd, data, size,
	     sock_errstr);
    }
    
    if (status) {
      size  -= status;
      data  += status;
    } else {
      THROW1(system_error,0,"file descriptor closed (%s)",
             sock_errstr);
    }
  }
}
/**
 * gras_trp_tcp_chunk_recv:
 *
 * Receive data on a TCP socket.
 */
void
gras_trp_tcp_chunk_recv(gras_socket_t sock,
			char *data,
			unsigned long int size,
			unsigned long int bufsize) {

  /* TCP sockets are in duplex mode, don't check direction */
  xbt_assert0(sock, "Cannot recv on an NULL socket");
  xbt_assert0(size >= 0, "Cannot receive a negative amount of data");
  xbt_assert0(bufsize>=size,"Not enough buffer size to receive that much data");
  
  while (size) {
    int status = 0;
    
    DEBUG3("read(%d, %p, %ld);", sock->sd, data, size);
    status = tcp_read(sock->sd, data, (size_t)bufsize);
    
    if (status < 0) {
      THROW4(system_error,0,"read(%d,%p,%d) failed: %s",
	     sock->sd, data, (int)size,
	     sock_errstr);
    }
    
    if (status) {
      size    -= status;
      bufsize -= status;
      data    += status;
    } else {
      gras_socket_close(sock);
      THROW0(system_error,0,"Socket closed by remote side");
    }
  }
}


/*
 * Returns the tcp protocol number from the network protocol data base.
 *
 * getprotobyname() is not thread safe. We need to lock it.
 */
static int TcpProtoNumber(void) {
  struct protoent *fetchedEntry;
  static int returnValue = 0;
  
  if(returnValue == 0) {
    fetchedEntry = getprotobyname("tcp");
    xbt_assert0(fetchedEntry, "getprotobyname(tcp) gave NULL");
    returnValue = fetchedEntry->p_proto;
  }
  
  return returnValue;
}

#ifdef HAVE_WINSOCK_H
#define RETSTR( x ) case x: return #x

const char *gras_wsa_err2string( int err ) {
   switch( err ) {
      RETSTR( WSAEINTR );
      RETSTR( WSAEBADF );
      RETSTR( WSAEACCES );
      RETSTR( WSAEFAULT );
      RETSTR( WSAEINVAL );
      RETSTR( WSAEMFILE );
      RETSTR( WSAEWOULDBLOCK );
      RETSTR( WSAEINPROGRESS );
      RETSTR( WSAEALREADY );
      RETSTR( WSAENOTSOCK );
      RETSTR( WSAEDESTADDRREQ );
      RETSTR( WSAEMSGSIZE );
      RETSTR( WSAEPROTOTYPE );
      RETSTR( WSAENOPROTOOPT );
      RETSTR( WSAEPROTONOSUPPORT );
      RETSTR( WSAESOCKTNOSUPPORT );
      RETSTR( WSAEOPNOTSUPP );
      RETSTR( WSAEPFNOSUPPORT );
      RETSTR( WSAEAFNOSUPPORT );
      RETSTR( WSAEADDRINUSE );
      RETSTR( WSAEADDRNOTAVAIL );
      RETSTR( WSAENETDOWN );
      RETSTR( WSAENETUNREACH );
      RETSTR( WSAENETRESET );
      RETSTR( WSAECONNABORTED );
      RETSTR( WSAECONNRESET );
      RETSTR( WSAENOBUFS );
      RETSTR( WSAEISCONN );
      RETSTR( WSAENOTCONN );
      RETSTR( WSAESHUTDOWN );
      RETSTR( WSAETOOMANYREFS );
      RETSTR( WSAETIMEDOUT );
      RETSTR( WSAECONNREFUSED );
      RETSTR( WSAELOOP );
      RETSTR( WSAENAMETOOLONG );
      RETSTR( WSAEHOSTDOWN );
      RETSTR( WSAEHOSTUNREACH );
      RETSTR( WSAENOTEMPTY );
      RETSTR( WSAEPROCLIM );
      RETSTR( WSAEUSERS );
      RETSTR( WSAEDQUOT );
      RETSTR( WSAESTALE );
      RETSTR( WSAEREMOTE );
      RETSTR( WSASYSNOTREADY );
      RETSTR( WSAVERNOTSUPPORTED );
      RETSTR( WSANOTINITIALISED );
      RETSTR( WSAEDISCON );
      
#ifdef HAVE_WINSOCK2
      RETSTR( WSAENOMORE );
      RETSTR( WSAECANCELLED );
      RETSTR( WSAEINVALIDPROCTABLE );
      RETSTR( WSAEINVALIDPROVIDER );
      RETSTR( WSASYSCALLFAILURE );
      RETSTR( WSASERVICE_NOT_FOUND );
      RETSTR( WSATYPE_NOT_FOUND );
      RETSTR( WSA_E_NO_MORE );
      RETSTR( WSA_E_CANCELLED );
      RETSTR( WSAEREFUSED );
#endif /* HAVE_WINSOCK2	*/

      RETSTR( WSAHOST_NOT_FOUND );
      RETSTR( WSATRY_AGAIN );
      RETSTR( WSANO_RECOVERY );
      RETSTR( WSANO_DATA );
   }
   return "unknown WSA error";
}
#endif /* HAVE_WINSOCK_H */
