/* $Id$ */

/* ping - ping/pong demo of GRAS features                                   */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <gras.h>

/* **********************************************************************
 * Comon code
 * **********************************************************************/
/* message definition */
#define MSG_PING 1024
#define MSG_PONG 1025

typedef struct {
  int dummy;
} msgPing_t;

static const DataDescriptor msgPingDesc[] = 
  {SIMPLE_MEMBER(INT_TYPE,1,offsetof(msgPing_t,dummy))};
#define msgPingLength 1

typedef msgPing_t msgPong_t;
static const DataDescriptor msgPongDesc[] = 
  {SIMPLE_MEMBER(INT_TYPE,1,offsetof(msgPong_t,dummy))};
#define msgPongLength 1

/* Function prototypes */
gras_error_t register_messages(const char *prefix);

/* Code */
gras_error_t register_messages(const char *prefix) {
  gras_error_t errcode;

  if ((errcode=gras_msgtype_register(MSG_PING,"ping",1,msgPingDesc,msgPingLength)) ||
      (errcode=gras_msgtype_register(MSG_PONG,"pong",1,msgPongDesc,msgPongLength))) {
    fprintf(stderr,"%s: Unable register the messages (got error %s)\n",
	    prefix,gras_error_name(errcode));
    return errcode;
  }
  return no_error;
}

/* **********************************************************************
 * Server code
 * **********************************************************************/

/* Global private data */
typedef struct {
  gras_sock_t *sock;
  int endcondition;
} server_data_t;

/* Function prototypes */
int server_cbPingHandler(gras_msg_t *msg);
int server (int argc,char *argv[]);


int server_cbPingHandler(gras_msg_t *msg) {
  int *dummy=malloc(sizeof(int*));
  server_data_t *g=(server_data_t*)gras_userdata_get();

  g->endcondition = 0;
  *dummy=4321;
  fprintf (stderr,"SERVER: >>>>>>>> Got message PING(%d) from %s:%d <<<<<<<<\n", 
	   gras_msg_ctn(msg,0,0,int), gras_sock_get_peer_name(msg->sock),gras_sock_get_peer_port(msg->sock));
  
  if (gras_msg_new_and_send(msg->sock,MSG_PONG,1,   dummy,1)) {
    fprintf(stderr,"SERVER: Unable answer with PONG\n");
    gras_sock_close(g->sock);
    return 1;
  }

  gras_msg_free(msg);
  fprintf(stderr,"SERVER: >>>>>>>> Answed with PONG(4321) <<<<<<<<\n");
  g->endcondition = 1;
  return 1;
}

int server (int argc,char *argv[]) {
  gras_error_t errcode;
  server_data_t *g=gras_userdata_new(server_data_t);  

  if ((errcode=gras_sock_server_open(4000,4000,&(g->sock)))) { 
    fprintf(stderr,"SERVER: Error %s encountered while opening the server socket\n",gras_error_name(errcode));
    return 1;
  }

  if (register_messages("SERVER") || 
      gras_cb_register(MSG_PING,-1,&server_cbPingHandler)) {
    gras_sock_close(g->sock);
    return 1;
  }

  fprintf(stderr,"SERVER: >>>>>>>> Listening on port %d <<<<<<<<\n",gras_sock_get_my_port(g->sock));
  g->endcondition=0;

  while (!g->endcondition) {
    if ((errcode=gras_msg_handle(60.0))) {
      fprintf(stderr,"SERVER: Error '%s' while handling message\n",gras_error_name(errcode));
      gras_sock_close(g->sock);
      return errcode;
    }
  }

  gras_sleep(5,0);
  fprintf(stderr,"SERVER: Done.\n");
  return gras_sock_close(g->sock);
}

/* **********************************************************************
 * Client code
 * **********************************************************************/

/* Global private data */
typedef struct {
  gras_sock_t *sock;
} client_data_t;

/* Function prototypes */
int client (int argc,char *argv[]);

int client(int argc,char *argv[]) {
  int dummy=1234;
  gras_msg_t *msg;
  gras_error_t errcode;
  client_data_t *g=gras_userdata_new(client_data_t);

  if ((errcode=gras_sock_client_open("102",4000,&(g->sock)))) {
    fprintf(stderr,"Client: Unable to connect to the server. Got %s\n",
	    gras_error_name(errcode));
    return 1;
  }

  if (register_messages("Client")) {
    //    grasCloseSocket(g->sock);
    return 1;
  }

  fprintf(stderr,"Client: >>>>>>>> Connected to server which is on %s:%d <<<<<<<<\n", 
	  gras_sock_get_peer_name(g->sock),gras_sock_get_peer_port(g->sock));

  if (gras_msg_new_and_send(g->sock,MSG_PING, 1,   &dummy,1)) {
    fprintf(stderr,"Client: Unable send PING to server\n");
    gras_sock_close(g->sock);
    return 1;
  }
  fprintf(stderr,"Client: >>>>>>>> Message PING(%d) sent to %s:%d <<<<<<<<\n",
	  dummy, gras_sock_get_peer_name(g->sock),gras_sock_get_peer_port(g->sock));

  if ((errcode=gras_msg_wait(6000,MSG_PONG,&msg))) {
    fprintf(stderr,"Client: Why can't I get my PONG message like everyone else (%s error)?!\n",
	    gras_error_name(errcode));
    gras_sock_close(g->sock);
    return 1;
  }

  fprintf(stderr,"Client: >>>>>>>> Message PONG(%d) got from %s:%d <<<<<<<<\n", 
	  gras_msg_ctn(msg,0,0,int),gras_sock_get_peer_name(msg->sock),gras_sock_get_peer_port(msg->sock));
  gras_msg_free(msg);

  gras_sleep(5,0);
  gras_sock_close(g->sock);
  fprintf(stderr,"Client: Done.\n");
  return 0;
}
