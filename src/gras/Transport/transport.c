/* $Id$ */

/* transport - low level communication                                      */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 Martin Quinson.                                       */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras/Transport/transport_private.h"

GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(transport,gras,"Conveying bytes over the network");

static gras_dict_t  *_gras_trp_plugins;     /* All registered plugins */
static void gras_trp_plugin_free(void *p); /* free one of the plugins */

static void gras_trp_socket_free(void *s); /* free one socket */

static void
gras_trp_plugin_new(const char *name, gras_trp_setup_t setup) {
  gras_error_t errcode;

  gras_trp_plugin_t *plug = gras_new0(gras_trp_plugin_t, 1);
  
  DEBUG1("Create plugin %s",name);

  plug->name=gras_strdup(name);

  errcode = setup(plug);
  switch (errcode) {
  case mismatch_error:
    /* SG plugin return mismatch when in RL mode (and vice versa) */
    gras_free(plug->name);
    gras_free(plug);
    break;

  case no_error:
    gras_dict_set(_gras_trp_plugins,
		  name, plug, gras_trp_plugin_free);
    break;

  default:
    DIE_IMPOSSIBLE;
  }

}

void gras_trp_init(void){
  /* make room for all plugins */
  _gras_trp_plugins=gras_dict_new();

  /* Add them */
  gras_trp_plugin_new("tcp", gras_trp_tcp_setup);
  gras_trp_plugin_new("file",gras_trp_file_setup);
  gras_trp_plugin_new("sg",gras_trp_sg_setup);

  /* buf is composed, so it must come after the others */
  gras_trp_plugin_new("buf", gras_trp_buf_setup);

}

void
gras_trp_exit(void){
  gras_dict_free(&_gras_trp_plugins);
}


void gras_trp_plugin_free(void *p) {
  gras_trp_plugin_t *plug = p;

  if (plug) {
    if (plug->exit) {
      plug->exit(plug);
    } else if (plug->data) {
      DEBUG1("Plugin %s lacks exit(). Free data anyway.",plug->name);
      gras_free(plug->data);
    }

    gras_free(plug->name);
    gras_free(plug);
  }
}


/**
 * gras_trp_socket_new:
 *
 * Malloc a new socket, and initialize it with defaults
 */
void gras_trp_socket_new(int incoming,
			 gras_socket_t **dst) {

  gras_socket_t *sock=gras_new(gras_socket_t,1);

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
  sock->raw = 0;

  *dst = sock;

  gras_dynar_push(gras_socketset_get(),dst);
}


/**
 * gras_socket_server_ext:
 *
 * Opens a server socket and make it ready to be listened to.
 * In real life, you'll get a TCP socket.
 */
gras_error_t
gras_socket_server_ext(unsigned short port,
		       
		       unsigned long int bufSize,
		       int raw,
		       
		       /* OUT */ gras_socket_t **dst) {
 
  gras_error_t errcode;
  gras_trp_plugin_t *trp;
  gras_socket_t *sock;

  *dst = NULL;

  DEBUG1("Create a server socket from plugin %s",gras_if_RL() ? "tcp" : "sg");
  TRY(gras_trp_plugin_get_by_name("buf",&trp));

  /* defaults settings */
  gras_trp_socket_new(1,&sock);
  sock->plugin= trp;
  sock->port=port;
  sock->bufSize = bufSize;
  sock->raw = raw;

  /* Call plugin socket creation function */
  errcode = trp->socket_server(trp, sock);
  DEBUG3("in=%c out=%c accept=%c",
	 sock->incoming?'y':'n', 
	 sock->outgoing?'y':'n',
	 sock->accepting?'y':'n');

  if (errcode != no_error) {
    gras_free(sock);
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
gras_error_t
gras_socket_client_ext(const char *host,
		       unsigned short port,
		       
		       unsigned long int bufSize,
		       int raw,
		       
		       /* OUT */ gras_socket_t **dst) {
 
  gras_error_t errcode;
  gras_trp_plugin_t *trp;
  gras_socket_t *sock;

  *dst = NULL;

  TRY(gras_trp_plugin_get_by_name("buf",&trp));

  DEBUG1("Create a client socket from plugin %s",gras_if_RL() ? "tcp" : "sg");
  /* defaults settings */
  gras_trp_socket_new(0,&sock);
  sock->plugin= trp;
  sock->peer_port = port;
  sock->peer_name = (char*)strdup(host?host:"localhost");
  sock->bufSize = bufSize;
  sock->raw = raw;

  /* plugin-specific */
  errcode= (*trp->socket_client)(trp, sock);
  DEBUG3("in=%c out=%c accept=%c",
	 sock->incoming?'y':'n', 
	 sock->outgoing?'y':'n',
	 sock->accepting?'y':'n');

  if (errcode != no_error) {
    gras_free(sock);
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
gras_error_t
gras_socket_server(unsigned short port,
		   /* OUT */ gras_socket_t **dst) {
   return gras_socket_server_ext(port,32,0,dst);
}

/**
 * gras_socket_client:
 *
 * Opens a client socket to a remote host.
 * In real life, you'll get a TCP socket.
 */
gras_error_t
gras_socket_client(const char *host,
		   unsigned short port,
		   /* OUT */ gras_socket_t **dst) {
   return gras_socket_client_ext(host,port,32,0,dst);
}


void gras_socket_close(gras_socket_t *sock) {
  gras_dynar_t *sockets = gras_socketset_get();
  gras_socket_t *sock_iter;
  int cursor;

  /* FIXME: Issue an event when the socket is closed */
  if (sock) {
    gras_dynar_foreach(sockets,cursor,sock_iter) {
      if (sock == sock_iter) {
	gras_dynar_cursor_rm(sockets,&cursor);
	if ( sock->plugin->socket_close) 
	  (* sock->plugin->socket_close)(sock);

	/* free the memory */
	if (sock->peer_name)
	  gras_free(sock->peer_name);
	gras_free(sock);
	return;
      }
    }
    WARN0("Ignoring request to free an unknown socket");
  }
}

/**
 * gras_trp_chunk_send:
 *
 * Send a bunch of bytes from on socket
 */
gras_error_t
gras_trp_chunk_send(gras_socket_t *sd,
		    char *data,
		    long int size) {
  gras_assert1(sd->outgoing,
	       "Socket not suited for data send (outgoing=%c)",
	       sd->outgoing?'y':'n');
  gras_assert1(sd->plugin->chunk_send,
	       "No function chunk_send on transport plugin %s",
	       sd->plugin->name);
  return (*sd->plugin->chunk_send)(sd,data,size);
}
/**
 * gras_trp_chunk_recv:
 *
 * Receive a bunch of bytes from a socket
 */
gras_error_t 
gras_trp_chunk_recv(gras_socket_t *sd,
		    char *data,
		    long int size) {
  gras_assert0(sd->incoming,
	       "Socket not suited for data receive");
  gras_assert1(sd->plugin->chunk_recv,
	       "No function chunk_recv on transport plugin %s",
	       sd->plugin->name);
  return (sd->plugin->chunk_recv)(sd,data,size);
}

/**
 * gras_trp_flush:
 *
 * Make sure all pending communications are done
 */
gras_error_t 
gras_trp_flush(gras_socket_t *sd) {
  return (sd->plugin->flush)(sd);
}

gras_error_t
gras_trp_plugin_get_by_name(const char *name,
			    gras_trp_plugin_t **dst){

  return gras_dict_get(_gras_trp_plugins,name,(void**)dst);
}

int   gras_socket_my_port  (gras_socket_t *sock) {
  return sock->port;
}
int   gras_socket_peer_port(gras_socket_t *sock) {
  return sock->peer_port;
}
char *gras_socket_peer_name(gras_socket_t *sock) {
  return sock->peer_name;
}

gras_error_t gras_socket_raw_send(gras_socket_t *peer, 
				  unsigned int timeout,
				  unsigned long int exp_size, 
				  unsigned long int msg_size) {
  gras_error_t errcode;
  char *chunk = gras_malloc(msg_size);
  int exp_sofar;
   
  gras_assert0(peer->raw,"Asked to send raw data on a regular socket\n");
  for (exp_sofar=0; exp_sofar < exp_size; exp_sofar += msg_size) {
     TRY(gras_trp_chunk_send(peer,chunk,msg_size));
  }
	     
  gras_free(chunk);
  return no_error;//gras_socket_raw_exchange(peer,1,timeout,expSize,msgSize);   
}

gras_error_t gras_socket_raw_recv(gras_socket_t *peer, 
				  unsigned int timeout,
				  unsigned long int exp_size, 
				  unsigned long int msg_size){
  
  gras_error_t errcode;
  char *chunk = gras_malloc(msg_size);
  int exp_sofar;

  gras_assert0(peer->raw,"Asked to recveive raw data on a regular socket\n");
  for (exp_sofar=0; exp_sofar < exp_size; exp_sofar += msg_size) {
     TRY(gras_trp_chunk_send(peer,chunk,msg_size));
  }

  gras_free(chunk);
  return no_error;//gras_socket_raw_exchange(peer,0,timeout,expSize,msgSize);   
}
