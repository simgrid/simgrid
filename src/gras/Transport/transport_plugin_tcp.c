/* $Id$ */

/* buf trp (transport) - buffered transport using the TCP one               */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
#include <string.h>       /* memset */

#include "portable.h"
#include "xbt/misc.h"
#include "xbt/sysdep.h"
#include "xbt/ex.h"
#include "gras/Transport/transport_private.h"
#include "gras/Msg/msg_interface.h" /* listener_close_socket */

/* FIXME maybe READV is sometime a good thing? */
#undef HAVE_READV

#ifdef HAVE_READV
#include <sys/uio.h>
#endif       

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras_trp_tcp,gras_trp,
      "TCP buffered transport");

/***
 *** Specific socket part
 ***/

typedef enum { buffering_buf, buffering_iov } buffering_kind;

typedef struct {
  int size;
  char *data;
  int pos; /* for receive; not exchanged over the net */
} gras_trp_buf_t;


struct gras_trp_bufdata_{
  int buffsize;
  gras_trp_buf_t in_buf;
  gras_trp_buf_t out_buf;

#ifdef HAVE_READV
  xbt_dynar_t in_buf_v;
  xbt_dynar_t out_buf_v;
#endif

  buffering_kind in;
  buffering_kind out;
};


/*****************************/
/****[ SOCKET MANAGEMENT ]****/
/*****************************/
/* we exchange port number on client side on socket creation,
   so we need to be able to talk right now. */
static XBT_INLINE void gras_trp_tcp_send(gras_socket_t sock, const char *data,
				     unsigned long int size);
static int gras_trp_tcp_recv(gras_socket_t sock, char *data,
			     unsigned long int size);


static int _gras_tcp_proto_number(void);

static XBT_INLINE void gras_trp_sock_socket_client(gras_trp_plugin_t ignored,
					       gras_socket_t sock){
  
  struct sockaddr_in addr;
  struct hostent *he;
  struct in_addr *haddr;
  int size = sock->buf_size; 
  uint32_t myport = htonl(((gras_trp_procdata_t) gras_libdata_by_id(gras_trp_libdata_id))->myport);

  sock->incoming = 1; /* TCP sockets are duplex'ed */

  sock->sd = socket (AF_INET, SOCK_STREAM, 0);
  
  if (sock->sd < 0) {
    THROW1(system_error,0, "Failed to create socket: %s", sock_errstr(sock_errno));
  }

  if (setsockopt(sock->sd, SOL_SOCKET, SO_RCVBUF, (char *)&size, sizeof(size)) ||
      setsockopt(sock->sd, SOL_SOCKET, SO_SNDBUF, (char *)&size, sizeof(size))) {
     VERB1("setsockopt failed, cannot set buffer size: %s",sock_errstr(sock_errno));
  }
  
  he = gethostbyname (sock->peer_name);
  if (he == NULL) {
    THROW2(system_error,0, "Failed to lookup hostname %s: %s",
	   sock->peer_name, sock_errstr(sock_errno));
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
	   sock->peer_name, sock->peer_port, sock_errstr(sock_errno));
  }

  gras_trp_tcp_send(sock,(char*)&myport,sizeof(uint32_t));
  DEBUG1("peerport sent to %d", sock->peer_port);

  VERB4("Connect to %s:%d (sd=%d, port %d here)",
	sock->peer_name, sock->peer_port, sock->sd, sock->port);
}

/**
 * gras_trp_sock_socket_server:
 *
 * Open a socket used to receive messages.
 */
static XBT_INLINE void gras_trp_sock_socket_server(gras_trp_plugin_t ignored,
					       gras_socket_t sock){
  int size = sock->buf_size; 
  int on = 1;
  struct sockaddr_in server;

  sock->outgoing  = 1; /* TCP => duplex mode */

  server.sin_port = htons((u_short)sock->port);
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_family = AF_INET;
  if((sock->sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    THROW1(system_error,0,"Socket allocation failed: %s", sock_errstr(sock_errno));

  if (setsockopt(sock->sd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)))
     THROW1(system_error,0,
	    "setsockopt failed, cannot condition the socket: %s",
	    sock_errstr(sock_errno));
   
  if (   setsockopt(sock->sd, SOL_SOCKET, SO_RCVBUF,
		    (char *)&size, sizeof(size)) 
      || setsockopt(sock->sd, SOL_SOCKET, SO_SNDBUF, 
		    (char *)&size, sizeof(size))) {
     VERB1("setsockopt failed, cannot set buffer size: %s",
	   sock_errstr(sock_errno));
  }
	
  if (bind(sock->sd, (struct sockaddr *)&server, sizeof(server)) == -1) {
    tcp_close(sock->sd);
    THROW2(system_error,0,
	   "Cannot bind to port %d: %s",
	   sock->port, sock_errstr(sock_errno));
  }

  DEBUG2("Listen on port %d (sd=%d)",sock->port, sock->sd);
  if (listen(sock->sd, 5) < 0) {
    tcp_close(sock->sd);
    THROW2(system_error,0,
	   "Cannot listen on port %d: %s",
	   sock->port,sock_errstr(sock_errno));
  }

  VERB2("Openned a server socket on port %d (sd=%d)",sock->port,sock->sd);
}

static gras_socket_t gras_trp_sock_socket_accept(gras_socket_t sock) {
  gras_socket_t res;
  
  struct sockaddr_in peer_in;
  socklen_t peer_in_len = sizeof(peer_in);

  int sd;
  int tmp_errno;
  int size;

  int i = 1;
  socklen_t s = sizeof(int);

  uint32_t hisport;
		
  XBT_IN;
  gras_trp_socket_new(1,&res);

  sd = accept(sock->sd, (struct sockaddr *)&peer_in, &peer_in_len);
  tmp_errno = sock_errno;

  if (sd == -1) {
    gras_socket_close(sock);
    THROW1(system_error,0,
	   "Accept failed (%s). Droping server socket.", sock_errstr(tmp_errno));
  }
  
  if (setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, (char *)&i, s) 
      || setsockopt(sd, _gras_tcp_proto_number(), TCP_NODELAY, (char *)&i, s))
    THROW1(system_error,0,"setsockopt failed, cannot condition the socket: %s",
	   sock_errstr(tmp_errno));

  res->buf_size = sock->buf_size;
  size = sock->buf_size;
  if (setsockopt(sd, SOL_SOCKET, SO_RCVBUF, (char *)&size, sizeof(size))
      || setsockopt(sd, SOL_SOCKET, SO_SNDBUF, (char *)&size, sizeof(size)))
    VERB1("setsockopt failed, cannot set buffer size: %s", sock_errstr(tmp_errno));
     
  res->plugin    = sock->plugin;
  res->incoming  = sock->incoming;
  res->outgoing  = sock->outgoing;
  res->accepting = 0;
  res->sd        = sd;
  res->port      = -1;

  gras_trp_tcp_recv(res,(char*)&hisport,sizeof(hisport));
  res->peer_port = ntohl(hisport);
  DEBUG1("peerport %d received",res->peer_port);

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
  xbt_dynar_push(((gras_trp_procdata_t)
		  gras_libdata_by_id(gras_trp_libdata_id))->sockets,&res);
   
  XBT_OUT;
  return res;
}

static void gras_trp_sock_socket_close(gras_socket_t sock){
  
  if (!sock) return; /* close only once */

  VERB1("close tcp connection %d", sock->sd);
   
  /* ask the listener to close the socket */
  gras_msg_listener_close_socket(sock->sd);
}
/************************************/
/****[ end of SOCKET MANAGEMENT ]****/
/************************************/


/************************************/
/****[ UNBUFFERED DATA EXCHANGE ]****/
/************************************/
/* Temptation to merge this with file data exchange is great, 
   but doesn't work on BillWare (see tcp_write() in portable.h) */
static XBT_INLINE void gras_trp_tcp_send(gras_socket_t sock,
				     const char *data,
				     unsigned long int size) {
  
  while (size) {
    int status = 0;
    
    status = tcp_write(sock->sd, data, (size_t)size);
    DEBUG3("write(%d, %p, %ld);", sock->sd, data, size);
    
    if (status < 0) {
#ifdef EWOULDBLOCK
       if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
#else
       if (errno == EINTR || errno == EAGAIN)
#endif
	 continue;
       
      THROW4(system_error,0,"write(%d,%p,%ld) failed: %s",
	     sock->sd, data, size,
	     sock_errstr(sock_errno));
    }
    
    if (status) {
      size  -= status;
      data  += status;
    } else {
      THROW1(system_error,0,"file descriptor closed (%s)",
             sock_errstr(sock_errno));
    }
  }
}
static XBT_INLINE int 
gras_trp_tcp_recv_withbuffer(gras_socket_t sock,
				   char *data,
				   unsigned long int size,
				   unsigned long int bufsize) {

  int got = 0;

  if (sock->recvd) {
     data[0] = sock->recvd_val;
     sock->recvd = 0;
     got++;
     bufsize--;
  }   

  while (size>got) {
    int status = 0;
    
    DEBUG5("read(%d, %p, %ld) got %d so far (%s)",
	  sock->sd, data+got, bufsize, got,
	  hexa_str((unsigned char*)data,got,0));
    status = tcp_read(sock->sd, data+got, (size_t)bufsize);
    
    if (status < 0) {
      THROW7(system_error,0,"read(%d,%p,%d) from %s:%d failed: %s; got %d so far",
	     sock->sd, data+got, (int)size,
	     gras_socket_peer_name(sock),gras_socket_peer_port(sock),
	     sock_errstr(sock_errno),
	     got);
    }
    DEBUG2("Got %d more bytes (%s)",status,hexa_str((unsigned char*)data+got,status,0));
    
    if (status) {
      bufsize -= status;
      got     += status;
    } else {
      THROW1(system_error,errno,"Socket closed by remote side (got %d bytes before this)",
	     got);
    }
  }

  return got;
}

static int gras_trp_tcp_recv(gras_socket_t sock,
				   char *data,
				   unsigned long int size) {
  return gras_trp_tcp_recv_withbuffer(sock,data,size,size);

}
/*******************************************/
/****[ end of UNBUFFERED DATA EXCHANGE ]****/
/*******************************************/

/**********************************/
/****[ BUFFERED DATA EXCHANGE ]****/
/**********************************/

/* Make sure the data is sent */
static void
gras_trp_bufiov_flush(gras_socket_t sock) {
#ifdef HAVE_READV
  xbt_dynar_t vect;
  int size;
#endif
  gras_trp_bufdata_t *data=sock->bufdata;
  XBT_IN;    
  
  DEBUG0("Flush");
  if (data->out == buffering_buf) {
    if (XBT_LOG_ISENABLED(gras_trp_tcp,xbt_log_priority_debug))
      hexa_print("chunk to send ",
		 (unsigned char *) data->out_buf.data,data->out_buf.size);
    if ((data->out_buf.size - data->out_buf.pos) != 0) { 
      DEBUG3("Send the chunk (size=%d) to %s:%d",data->out_buf.size,
	     gras_socket_peer_name(sock),gras_socket_peer_port(sock));
      gras_trp_tcp_send(sock, data->out_buf.data, data->out_buf.size);
      VERB1("Chunk sent (size=%d)",data->out_buf.size);
      data->out_buf.size = 0;
    }
  }

#ifdef HAVE_READV
  if (data->out == buffering_iov) {
    DEBUG0("Flush out iov");
    vect = sock->bufdata->out_buf_v;
    if ((size = xbt_dynar_length(vect))) {
      DEBUG1("Flush %d chunks out of this socket",size);
      writev(sock->sd,xbt_dynar_get_ptr(vect,0),size);
      xbt_dynar_reset(vect);
    }
    data->out_buf.size = 0; /* reset the buffer containing non-stable data */
  }

  if (data->in == buffering_iov) {
    DEBUG0("Flush in iov");
    vect = sock->bufdata->in_buf_v;
    if ((size = xbt_dynar_length(vect))) {
      DEBUG1("Get %d chunks from of this socket",size);
      readv(sock->sd,xbt_dynar_get_ptr(vect,0),size);
      xbt_dynar_reset(vect);
    }
  }
#endif
}
static void
gras_trp_buf_send(gras_socket_t sock,
		  const char *chunk,
		  unsigned long int size,
		  int stable_ignored) {

  gras_trp_bufdata_t *data=(gras_trp_bufdata_t*)sock->bufdata;
  int chunk_pos=0;

  XBT_IN;

  while (chunk_pos < size) {
    /* size of the chunk to receive in that shot */
    long int thissize = min(size-chunk_pos,data->buffsize-data->out_buf.size);
    DEBUG4("Set the chars %d..%ld into the buffer; size=%ld, ctn=(%s)",
	   (int)data->out_buf.size,
	   ((int)data->out_buf.size) + thissize -1,
	   size,
	   hexa_str((unsigned char*)chunk,thissize,0));

    memcpy(data->out_buf.data + data->out_buf.size, chunk + chunk_pos, thissize);

    data->out_buf.size += thissize;
    chunk_pos      += thissize;
    DEBUG4("New pos = %d; Still to send = %ld of %ld; ctn sofar=(%s)",
	   data->out_buf.size,size-chunk_pos,size,hexa_str((unsigned char*)chunk,chunk_pos,0));

    if (data->out_buf.size == data->buffsize) /* out of space. Flush it */
      gras_trp_bufiov_flush(sock);
  }

  XBT_OUT;
}

static int
gras_trp_buf_recv(gras_socket_t sock,
		  char *chunk,
		  unsigned long int size) {

  gras_trp_bufdata_t *data=sock->bufdata;
  long int chunk_pos = 0;
   
  XBT_IN;

  while (chunk_pos < size) {
    /* size of the chunk to receive in that shot */
    long int thissize;

    if (data->in_buf.size == data->in_buf.pos) { /* out of data. Get more */

      DEBUG2("Get more data (size=%d,bufsize=%d)",
	     (int)MIN(size-chunk_pos,data->buffsize),
	     (int)data->buffsize);

       
      data->in_buf.size = 
	gras_trp_tcp_recv_withbuffer(sock, data->in_buf.data, 
				     MIN(size-chunk_pos,data->buffsize),
				     data->buffsize);
       
      data->in_buf.pos=0;
    }
     
    thissize = min(size-chunk_pos ,  data->in_buf.size - data->in_buf.pos);
    memcpy(chunk+chunk_pos, data->in_buf.data + data->in_buf.pos, thissize);

    data->in_buf.pos += thissize;
    chunk_pos        += thissize;
    DEBUG4("New pos = %d; Still to receive = %ld of %ld. Ctn so far=(%s)",
	   data->in_buf.pos,size - chunk_pos,size,hexa_str((unsigned char*)chunk,chunk_pos,0));
  }
  /* indicate on need to the gras_select function that there is more to read on this socket so that it does not actually select */
  sock->moredata = (data->in_buf.size > data->in_buf.pos);
  DEBUG1("There is %smore data",(sock->moredata?"":"no "));
   
  XBT_OUT;
  return chunk_pos;
}

/*****************************************/
/****[ end of BUFFERED DATA EXCHANGE ]****/
/*****************************************/

/********************************/
/****[ VECTOR DATA EXCHANGE ]****/
/********************************/
#ifdef HAVE_READV
static void
gras_trp_iov_send(gras_socket_t sock,
		  const char *chunk,
		  unsigned long int size,
		  int stable) {
  struct iovec elm;
  gras_trp_bufdata_t *data=(gras_trp_bufdata_t*)sock->bufdata;
    

  DEBUG1("Buffer one chunk to be sent later (%s)",
	hexa_str((char*)chunk,size,0));

  elm.iov_len = (size_t)size;

  if (!stable) {
    /* data storage won't last until flush. Save it in a buffer if we can */

    if (size > data->buffsize-data->out_buf.size) {
      /* buffer too small: 
	 flush the socket, using data in its actual storage */
      elm.iov_base = (void*)chunk;
      xbt_dynar_push(data->out_buf_v,&elm);

      gras_trp_bufiov_flush(sock);      
      return;
    } else {
      /* buffer big enough: 
	 copy data into it, and chain it for upcoming writev */
      memcpy(data->out_buf.data + data->out_buf.size, chunk, size);
      elm.iov_base = (void*)(data->out_buf.data + data->out_buf.size);
      data->out_buf.size += size;

      xbt_dynar_push(data->out_buf_v,&elm);
    }

  } else {
    /* data storage stable. Chain it */
    
    elm.iov_base = (void*)chunk;
    xbt_dynar_push(data->out_buf_v,&elm);
  }
}
static int
gras_trp_iov_recv(gras_socket_t sock,
		  char *chunk,
		  unsigned long int size) {
  struct iovec elm;

  DEBUG0("Buffer one chunk to be received later");
  elm.iov_base = (void*)chunk;
  elm.iov_len = (size_t)size;
  xbt_dynar_push(sock->bufdata->in_buf_v,&elm);

  return size;
}

#endif
/***************************************/
/****[ end of VECTOR DATA EXCHANGE ]****/
/***************************************/


/***
 *** Prototypes of BUFFERED
 ***/
   
void gras_trp_buf_socket_client(gras_trp_plugin_t self,
				gras_socket_t sock);
void gras_trp_buf_socket_server(gras_trp_plugin_t self,
				gras_socket_t sock);
gras_socket_t gras_trp_buf_socket_accept(gras_socket_t sock);

void         gras_trp_buf_socket_close(gras_socket_t sd);
  

gras_socket_t gras_trp_buf_init_sock(gras_socket_t sock) {
  gras_trp_bufdata_t *data=xbt_new(gras_trp_bufdata_t,1);
  
  data->buffsize = 100 * 1024 ; /* 100k */ 

  data->in_buf.size  = 0;
  data->in_buf.data  = xbt_malloc(data->buffsize);
  data->in_buf.pos   = 0; /* useless, indeed, since size==pos */
   
  data->out_buf.size = 0;
  data->out_buf.data = xbt_malloc(data->buffsize);
  data->out_buf.pos  = data->out_buf.size;

#ifdef HAVE_READV
  data->in_buf_v = data->out_buf_v = NULL;
  data->in_buf_v=xbt_dynar_new(sizeof(struct iovec),NULL);
  data->out_buf_v=xbt_dynar_new(sizeof(struct iovec),NULL);
  data->out = buffering_iov;
#else
  data->out = buffering_buf;
#endif
   
  data->in = buffering_buf;

  sock->bufdata = data;
  return sock;
}

/***
 *** Code
 ***/
void
gras_trp_tcp_setup(gras_trp_plugin_t plug) {

  plug->socket_client = gras_trp_buf_socket_client;
  plug->socket_server = gras_trp_buf_socket_server;
  plug->socket_accept = gras_trp_buf_socket_accept;
  plug->socket_close  = gras_trp_buf_socket_close;

#ifdef HAVE_READV
  plug->send = gras_trp_iov_send;
#else
  plug->send = gras_trp_buf_send;
#endif
  plug->recv = gras_trp_buf_recv;

  plug->raw_send    = gras_trp_tcp_send;
  plug->raw_recv    = gras_trp_tcp_recv;

  plug->flush         = gras_trp_bufiov_flush;

  plug->data = NULL;
  plug->exit = NULL;
}

void gras_trp_buf_socket_client(gras_trp_plugin_t self,
				/* OUT */ gras_socket_t sock){

  gras_trp_sock_socket_client(NULL,sock);
  gras_trp_buf_init_sock(sock);
}

/**
 * gras_trp_buf_socket_server:
 *
 * Open a socket used to receive messages.
 */
void gras_trp_buf_socket_server(gras_trp_plugin_t self,
				/* OUT */ gras_socket_t sock){

  gras_trp_sock_socket_server(NULL,sock);
  gras_trp_buf_init_sock(sock);
}

gras_socket_t gras_trp_buf_socket_accept(gras_socket_t sock) {
  return gras_trp_buf_init_sock(gras_trp_sock_socket_accept(sock));
}

void gras_trp_buf_socket_close(gras_socket_t sock){
  gras_trp_bufdata_t *data=sock->bufdata;

  if (data->in_buf.size!=data->in_buf.pos) {
     WARN3("Socket closed, but %d bytes were unread (size=%d,pos=%d)",
	   data->in_buf.size - data->in_buf.pos,
	   data->in_buf.size, data->in_buf.pos);
  }
  if (data->in_buf.data)
    free(data->in_buf.data);
   
  if (data->out_buf.size!=data->out_buf.pos) {
    DEBUG2("Flush the socket before closing (in=%d,out=%d)",
	   data->in_buf.size, data->out_buf.size);
    gras_trp_bufiov_flush(sock);
  }   
  if (data->out_buf.data)
    free(data->out_buf.data);

#ifdef HAVE_READV
  if (data->in_buf_v) {
    if (xbt_dynar_length(data->in_buf_v)) 
      WARN0("Socket closed, but some bytes were unread");
    xbt_dynar_free(&data->in_buf_v);
  }
  if (data->out_buf_v) {
    if (xbt_dynar_length(data->out_buf_v)) {
      DEBUG0("Flush the socket before closing");
      gras_trp_bufiov_flush(sock);
    }
    xbt_dynar_free(&data->out_buf_v);
  }
#endif

  free(data);
  gras_trp_sock_socket_close(sock);
}

/****************************/
/****[ HELPER FUNCTIONS ]****/
/****************************/

/*
 * Returns the tcp protocol number from the network protocol data base.
 *
 * getprotobyname() is not thread safe. We need to lock it.
 */
static int _gras_tcp_proto_number(void) {
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

/***********************************/
/****[ end of HELPER FUNCTIONS ]****/
/***********************************/
