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
					unsigned int bufSize, 
					/* OUT */ gras_socket_t **dst);
gras_error_t gras_trp_tcp_socket_server(gras_trp_plugin_t *self,
					unsigned short port,
					unsigned int bufSize, 
					/* OUT */ gras_socket_t **dst);
gras_error_t gras_trp_tcp_socket_accept(gras_socket_t  *sock,
					gras_socket_t **dst);

void         gras_trp_tcp_socket_close(gras_socket_t *sd);
  
gras_error_t gras_trp_tcp_bloc_send(gras_socket_t *sd,
				    char *data,
				    size_t size);

gras_error_t gras_trp_tcp_bloc_recv(gras_socket_t *sd,
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

  res->socket_client = gras_trp_tcp_socket_client;
  res->socket_server = gras_trp_tcp_socket_server;
  res->socket_accept = gras_trp_tcp_socket_accept;
  res->socket_close  = gras_trp_tcp_socket_close;

  res->bloc_send     = gras_trp_tcp_bloc_send;
  res->bloc_recv     = gras_trp_tcp_bloc_recv;

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
					unsigned int bufSize, 
					/* OUT */ gras_socket_t **dst){
  /*
  int addrCount;
  IPAddress addresses[10];
  int i;
  int sd;
  
  if (!(*sock=malloc(sizeof(gras_socket_t)))) {
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
  */
  RAISE_UNIMPLEMENTED;
}

/**
 * gras_trp_tcp_socket_server:
 *
 * Open a socket used to receive messages. bufSize is in ko.
 */
gras_error_t gras_trp_tcp_socket_server(gras_trp_plugin_t *self,
					unsigned short port,
					unsigned int bufSize, 
					/* OUT */ gras_socket_t **dst){
  int size = bufSize * 1024;
  int on = 1;
  int sd = -1;
  struct sockaddr_in server;

  gras_socket_t *res;
  gras_trp_tcp_specific_t *data=(gras_trp_tcp_specific_t*)self -> specific;
 
  res=malloc(sizeof(gras_socket_t));
  if (!res)
    RAISE_MALLOC;

  server.sin_port = htons((u_short)port);
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_family = AF_INET;
  if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    free(res);
    RAISE0(system_error,"socket allocation failed");
  }

  (void)setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
  (void)setsockopt(sd, SOL_SOCKET, SO_RCVBUF, (char *)&size, sizeof(size));
  (void)setsockopt(sd, SOL_SOCKET, SO_SNDBUF, (char *)&size, sizeof(size));
  if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1) {
    free(res);
    close(sd);
    RAISE1(system_error,"Cannot bind to port %d",port);
  }

  if (listen(sd, 5) != -1) {
    free(res);
    close(sd);
    RAISE1(system_error,"Cannot listen to port %d",port);
  }

  FD_SET(sd, &(data->incoming_socks));

  *dst=res;
  res->plugin = self;
  res->incoming = 1;
  res->sd = sd;
  res->port=port;
  res->peer_port=-1;
  res->peer_name=NULL;

  DEBUG2("Openned a server socket on port %d (sock %d)",port,sd);
  
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
    gras_socket_close(sock);
    RAISE1(system_error,
	   "Accept failed (%s). Droping server socket.", strerror(tmp_errno));
  } else {
    int i = 1;
    socklen_t s = sizeof(int);
  
    if (setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, (char *)&i, s) 
	|| setsockopt(sd, TcpProtoNumber(), TCP_NODELAY, (char *)&i, s)) {
      WARNING0("setsockopt failed, cannot condition the accepted socket");
    }
 
    i = ((gras_trp_tcp_sock_specific_t*)sock->specific)->buffsize;
    if (setsockopt(sd, SOL_SOCKET, SO_RCVBUF, (char *)&i, s)
	|| setsockopt(sd, SOL_SOCKET, SO_SNDBUF, (char *)&i, s)) {
      WARNING0("setsockopt failed, cannot set buffsize");	
    }
 
    res->plugin = sock->plugin;
    res->incoming = 1;
    res->sd = sd;
    res->port= -1;
    res->peer_port= peer_in.sin_port;

    if (((struct sockaddr *)&peer_in)->sa_family != AF_INET) {
      res->peer_name = strdup("unknown");
    } else {
      struct in_addr addrAsInAddr;
      char *tmp;
 
      addrAsInAddr.s_addr = peer_in.sin_addr.s_addr;
      
      tmp = inet_ntoa(addrAsInAddr);
      if (tmp != NULL) {
	res->peer_name = strdup(inet_ntoa(addrAsInAddr));
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

  /* close the socket */
  if(close(sock->sd) < 0) {
    WARNING3("error while closing tcp socket %d: %d (%s)\n", sock->sd, errno, strerror(errno));
  }

  /* forget about it */
  FD_CLR(sock->sd, &(tcp->incoming_socks));

}

gras_error_t gras_trp_tcp_bloc_send(gras_socket_t *sock,
				    char *data,
				    size_t size){

  gras_assert0(sock && !sock->incoming, "Ascked to send stuff on an incomming socket");
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

gras_error_t gras_trp_tcp_bloc_recv(gras_socket_t *sock,
				    char *data,
				    size_t size){

  gras_assert0(sock && !sock->incoming, "Ascked to receive stuff on an outcomming socket");
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
