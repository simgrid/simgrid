/* $Id$ */

/* tcp trp (transport) - send/receive a bunch of bytes from a tcp socket    */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 Martin Quinson.                                       */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include <unistd.h>       /* close() pipe() read() write() */
#include <signal.h>       /* close() pipe() read() write() */
#include <netinet/in.h>   /* sometimes required for #include <arpa/inet.h> */
#include <netinet/tcp.h>  /* TCP_NODELAY */
#include <arpa/inet.h>    /* inet_ntoa() */
#include <netdb.h>        /* getprotobyname() */
#include <sys/time.h>     /* struct timeval */
#include <errno.h>        /* errno */
#include <sys/wait.h>     /* waitpid() */
#include <sys/socket.h>   /* getpeername() socket() */
#include <stdlib.h>
#include <string.h>       /* memset */

#include "gras_private.h"
#include "transport_private.h"

GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(trp_tcp,transport);

typedef struct {
  int buffsize;
} gras_trp_tcp_sock_specific_t;

/***
 *** Prototypes 
 ***/
gras_error_t gras_trp_tcp_socket_client(gras_trp_plugin_t *self,
					const char *host,
					unsigned short port,
					/* OUT */ gras_socket_t *sock);
gras_error_t gras_trp_tcp_socket_server(gras_trp_plugin_t *self,
					unsigned short port,
					/* OUT */ gras_socket_t *sock);
gras_error_t gras_trp_tcp_socket_accept(gras_socket_t  *sock,
					gras_socket_t **dst);

void         gras_trp_tcp_socket_close(gras_socket_t *sd);
  
gras_error_t gras_trp_tcp_chunk_send(gras_socket_t *sd,
				     char *data,
				     size_t size);

gras_error_t gras_trp_tcp_chunk_recv(gras_socket_t *sd,
				     char *data,
				     size_t size);

void         gras_trp_tcp_free_specific(void *s);


static int TcpProtoNumber(void);
/***
 *** Specific plugin part
 ***/

typedef struct {
  fd_set incoming_socks;
} gras_trp_tcp_specific_t;

/***
 *** Specific socket part
 ***/


/***
 *** Code
 ***/
gras_error_t
gras_trp_tcp_init(gras_trp_plugin_t **dst) {

  gras_trp_plugin_t *res=malloc(sizeof(gras_trp_plugin_t));
  gras_trp_tcp_specific_t *tcp = malloc(sizeof(gras_trp_tcp_specific_t));
  if (!res || !tcp)
    RAISE_MALLOC;

  FD_ZERO(&(tcp->incoming_socks));

  res->name = strdup("TCP");
  res->socket_client = gras_trp_tcp_socket_client;
  res->socket_server = gras_trp_tcp_socket_server;
  res->socket_accept = gras_trp_tcp_socket_accept;
  res->socket_close  = gras_trp_tcp_socket_close;

  res->chunk_send    = gras_trp_tcp_chunk_send;
  res->chunk_recv    = gras_trp_tcp_chunk_recv;

  res->specific      = (void*)tcp;
  res->free_specific = gras_trp_tcp_free_specific;

  *dst = res;
  return no_error;
}

void gras_trp_tcp_free_specific(void *s) {
  gras_trp_tcp_specific_t *specific = s;
  free(specific);
}

gras_error_t gras_trp_tcp_socket_client(gras_trp_plugin_t *self,
					const char *host,
					unsigned short port,
					/* OUT */ gras_socket_t *sock){
  
  struct sockaddr_in addr;
  struct hostent *he;
  struct in_addr *haddr;

  sock->incoming = 1; /* TCP sockets are duplex'ed */

  sock->sd = socket (AF_INET, SOCK_STREAM, 0);
  
  if (sock->sd < 0) {
    RAISE1(system_error,
	   "Failed to create socket: %s",
	   strerror (errno));
  }
  
  he = gethostbyname (host);
  if (he == NULL) {
    RAISE2(system_error,
	   "Failed to lookup hostname %s: %s",
	   host, strerror (errno));
  }
  
  haddr = ((struct in_addr *) (he->h_addr_list)[0]);
  
  memset(&addr, 0, sizeof(struct sockaddr_in));
  memcpy (&addr.sin_addr, haddr, sizeof(struct in_addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons (port);

  if (connect (sock->sd, (struct sockaddr*) &addr, sizeof (addr)) < 0) {
    close(sock->sd);
    RAISE3(system_error,
	   "Failed to connect socket to %s:%d (%s)",
	   host, port, strerror (errno));
  }
  
  return no_error;
}

/**
 * gras_trp_tcp_socket_server:
 *
 * Open a socket used to receive messages.
 */
gras_error_t gras_trp_tcp_socket_server(gras_trp_plugin_t *self,
					unsigned short port,
					/* OUT */ gras_socket_t *sock){
//  int size = bufSize * 1024;
  int on = 1;
  struct sockaddr_in server;

  gras_trp_tcp_specific_t *data=(gras_trp_tcp_specific_t*)self -> specific;
 
  sock->outgoing  = 1; /* TCP => duplex mode */

  server.sin_port = htons((u_short)port);
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_family = AF_INET;
  if((sock->sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    RAISE1(system_error,"socket allocation failed: %s", strerror(errno));
  }

  (void)setsockopt(sock->sd, SOL_SOCKET, SO_REUSEADDR, 
		   (char *)&on, sizeof(on));
   /*
  (void)setsockopt(sd, SOL_SOCKET, SO_RCVBUF, (char *)&size, sizeof(size));
  (void)setsockopt(sd, SOL_SOCKET, SO_SNDBUF, (char *)&size, sizeof(size));
    */
  if (bind(sock->sd, (struct sockaddr *)&server, sizeof(server)) == -1) {
    close(sock->sd);
    RAISE2(system_error,"Cannot bind to port %d: %s",port, strerror(errno));
  }

  if (listen(sock->sd, 5) < 0) {
    close(sock->sd);
    RAISE2(system_error,"Cannot listen to port %d: %s",port,strerror(errno));
  }

  FD_SET(sock->sd, &(data->incoming_socks));

  DEBUG2("Openned a server socket on port %d (sock %d)",port,sock->sd);
  
  return no_error;
}

gras_error_t
gras_trp_tcp_socket_accept(gras_socket_t  *sock,
			   gras_socket_t **dst) {
  gras_socket_t *res;
  
  struct sockaddr_in peer_in;
  socklen_t peer_in_len = sizeof(peer_in);

  int sd;
  int tmp_errno;
				
  res=malloc(sizeof(gras_socket_t));
  if (!res)
    RAISE_MALLOC;

  sd = accept(sock->sd, (struct sockaddr *)&peer_in, &peer_in_len);
  tmp_errno = errno;

  if(sd == -1) {
    gras_socket_close(&sock);
    RAISE1(system_error,
	   "Accept failed (%s). Droping server socket.", strerror(tmp_errno));
  } else {
    int i = 1;
    socklen_t s = sizeof(int);
  
    if (setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, (char *)&i, s) 
	|| setsockopt(sd, TcpProtoNumber(), TCP_NODELAY, (char *)&i, s)) {
      WARNING0("setsockopt failed, cannot condition the accepted socket");
    }
 
     /* FIXME: bufSize removed until we can have optionsets 
    i = ((gras_trp_tcp_sock_specific_t*)sock->specific)->buffsize;
    if (setsockopt(sd, SOL_SOCKET, SO_RCVBUF, (char *)&i, s)
	|| setsockopt(sd, SOL_SOCKET, SO_SNDBUF, (char *)&i, s)) {
      WARNING0("setsockopt failed, cannot set buffsize");	
    }
      */
     
    res->plugin    = sock->plugin;
    res->incoming  = sock->incoming;
    res->outgoing  = sock->outgoing;
    res->accepting = 0;
    res->sd        = sd;
    res->port      = -1;
    res->peer_port = peer_in.sin_port;

    /* FIXME: Lock to protect inet_ntoa */
    if (((struct sockaddr *)&peer_in)->sa_family != AF_INET) {
      res->peer_name = strdup("unknown");
    } else {
      struct in_addr addrAsInAddr;
      char *tmp;
 
      addrAsInAddr.s_addr = peer_in.sin_addr.s_addr;
      
      tmp = inet_ntoa(addrAsInAddr);
      if (tmp != NULL) {
	res->peer_name = strdup(tmp);
      } else {
	res->peer_name = strdup("unknown");
      }
    }

    VERB3("accepted socket %d to %s:%d\n", sd, res->peer_name,res->peer_port);
    
    *dst = res;

    return no_error;
  }
}

void gras_trp_tcp_socket_close(gras_socket_t *sock){
  gras_trp_tcp_specific_t *tcp;
  
  if (!sock) return; /* close only once */
  tcp=sock->plugin->specific;

  DEBUG1("close tcp connection %d\n", sock->sd);

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

  /* forget about the socket */
  FD_CLR(sock->sd, &(tcp->incoming_socks));

  /* close the socket */
  if(close(sock->sd) < 0) {
    WARNING3("error while closing tcp socket %d: %d (%s)\n", 
	     sock->sd, errno, strerror(errno));
  }
}

/**
 * gras_trp_tcp_chunk_send:
 *
 * Send data on a TCP socket
 */
gras_error_t 
gras_trp_tcp_chunk_send(gras_socket_t *sock,
		    char *data,
		    size_t size) {
  
  /* TCP sockets are in duplex mode, don't check direction */
  gras_assert0(size >= 0, "Cannot send a negative amount of data");

  while (size) {
    int status = 0;
    
    status = write(sock->sd, data, (size_t)size);
    DEBUG3("write(%d, %p, %ld);\n", sock->sd, data, size);
    
    if (status == -1) {
      RAISE4(system_error,"write(%d,%p,%d) failed: %s",
	     sock->sd, data, (int)size,
	     strerror(errno));
    }
    
    if (status) {
      size  -= status;
      data  += status;
    } else {
      RAISE0(system_error,"file descriptor closed");
    }
  }

  return no_error;
}
/**
 * gras_trp_tcp_chunk_recv:
 *
 * Receive data on a TCP socket.
 */
gras_error_t 
gras_trp_tcp_chunk_recv(gras_socket_t *sock,
			char *data,
			size_t size) {

  /* TCP sockets are in duplex mode, don't check direction */
  gras_assert0(sock, "Cannot recv on an NULL socket");
  gras_assert0(size >= 0, "Cannot receive a negative amount of data");
  
  while (size) {
    int status = 0;
    
    status = read(sock->sd, data, (size_t)size);
    DEBUG3("read(%d, %p, %ld);\n", sock->sd, data, size);
    
    if (status == -1) {
      RAISE4(system_error,"read(%d,%p,%d) failed: %s",
	     sock->sd, data, (int)size,
	     strerror(errno));
    }
    
    if (status) {
      size  -= status;
      data  += status;
    } else {
      RAISE0(system_error,"file descriptor closed");
    }
  }
  
  return no_error;
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
    gras_assert0(fetchedEntry, "getprotobyname(tcp) gave NULL");
    returnValue = fetchedEntry->p_proto;
  }
  
  return returnValue;
}
