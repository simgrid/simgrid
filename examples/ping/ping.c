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

GRAS_LOG_NEW_DEFAULT_CATEGORY(Ping);

/* **********************************************************************
 * Comon code
 * **********************************************************************/

typedef struct {
  int dummy;
} msg_ping_t;

/* Function prototypes */
gras_error_t register_messages(void);

/* Code */
gras_error_t register_messages(void) {
  gras_error_t errcode;
  gras_datadesc_type_t *payload_t;
  gras_msgtype_t *msg_t;

  TRY(gras_datadesc_declare_struct("msg_ping_t",&payload_t));
  TRY(gras_datadesc_declare_struct_append(payload_t,"dummy",
					  gras_datadesc_by_name("int")));

  TRY(gras_msgtype_declare("ping", payload_t, &msg_t)); msg_t=NULL;
  TRY(gras_msgtype_declare("pong", payload_t, &msg_t));

  return no_error;
}

/* **********************************************************************
 * Server code
 * **********************************************************************/

/* Global private data */
typedef struct {
  gras_socket_t *sock;
  int endcondition;
} server_data_t;

/* Function prototypes */
int server_cb_ping_handler(gras_socket_t        *expeditor,
			   gras_datadesc_type_t *payload_type,
			   void                 *payload_data);
int server (int argc,char *argv[]);


int server_cb_ping_handler(gras_socket_t        *expeditor,
			   gras_datadesc_type_t *payload_type,
			   void                 *payload_data) {
			      
  gras_error_t errcode;
  msg_ping_t *msg=payload_data;
  gras_msgtype_t *pong_t;

  server_data_t *g=(server_data_t*)gras_userdata_get();

  g->endcondition = 0;
  INFO3("SERVER: >>>>>>>> Got message PING(%d) from %s:%d <<<<<<<<\n", 
	msg->dummy, 
 	gras_socket_peer_name(expeditor),
	gras_socket_peer_port(expeditor));
  
  msg->dummy = 4321;
  TRY(gras_msgtype_by_name("pong",&pong_t));
  errcode = gras_msg_send(expeditor, pong_t, payload_data);

  free(payload_data);
  if (errcode != no_error) {
    ERROR1("SERVER: Unable answer with PONG: %s\n", gras_error_name(errcode));
    gras_socket_close(&(g->sock));
    return 1;
  }

  free(msg);
  INFO0("SERVER: >>>>>>>> Answed with PONG(4321) <<<<<<<<\n");
  g->endcondition = 1;
  return 1;
}

int server (int argc,char *argv[]) {
  gras_error_t errcode;
  server_data_t *g=gras_userdata_new(server_data_t);  
  gras_msgtype_t *ping_msg;

  int port = 4000;
  
  if (argc == 2) {
    port=atoi(argv[1]);
  }

  if ((errcode=gras_socket_server(port,&(g->sock)))) { 
    CRITICAL1("Error %s encountered while opening the server socket",
	      gras_error_name(errcode));
    return 1;
  }

  TRYFAIL(register_messages());
  TRYFAIL(gras_msgtype_by_name("ping",&ping_msg));
  TRYFAIL(gras_cb_register(ping_msg,&server_cb_ping_handler));

  INFO1("SERVER: >>>>>>>> Listening on port %d <<<<<<<<",
	gras_socket_my_port(g->sock));
  g->endcondition=0;

  while (1) {
    errcode = gras_msg_handle(60.0);
    if (errcode != no_error && errcode != timeout_error) 
      return errcode;
    if (g->endcondition)
      break;
  }

  gras_sleep(5,0);
  INFO0("SERVER: Done.");
  gras_socket_close(&(g->sock));
  return no_error;
}

/* **********************************************************************
 * Client code
 * **********************************************************************/

/* Global private data */
typedef struct {
  gras_socket_t *sock;
} client_data_t;

/* Function prototypes */
int client (int argc,char *argv[]);

int client(int argc,char *argv[]) {
  gras_error_t errcode;
  client_data_t *g=gras_userdata_new(client_data_t);

  gras_socket_t  *from;
  msg_ping_t     *msg_ping_data, *msg_pong_data;
  gras_msgtype_t *msg_ping_type, *msg_pong_type;

  const char *host = "127.0.0.1";
        int   port = 4000;

  if (argc == 3) {
    host=argv[1];
    port=atoi(argv[2]);
  } 

  if ((errcode=gras_socket_client(host,port,&(g->sock)))) {
    fprintf(stderr,"Client: Unable to connect to the server. Got %s\n",
	    gras_error_name(errcode));
    return 1;
  }
  fprintf(stderr,"Client: Connected to %s:%d.\n",host,port);    


  TRY(register_messages());
  TRY(gras_msgtype_by_name("ping",&msg_ping_type));
  TRY(gras_msgtype_by_name("pong",&msg_pong_type));

  fprintf(stderr,
	  "Client: >>>>>>>> Connected to server which is on %s:%d <<<<<<<<\n", 
	  gras_socket_peer_name(g->sock),gras_socket_peer_port(g->sock));

  msg_ping_data = malloc(sizeof(msg_ping_t));
  msg_ping_data->dummy = 1234;
  errcode = gras_msg_send(g->sock, msg_ping_type, msg_ping_data);
  if (errcode != no_error) {
    fprintf(stderr, "Client: Unable send PING to server (%s)\n",
	    gras_error_name(errcode));
    gras_socket_close(&(g->sock));
    return 1;
  }
  fprintf(stderr,"Client: >>>>>>>> Message PING(%d) sent to %s:%d <<<<<<<<\n",
	  msg_ping_data->dummy,
	  gras_socket_peer_name(g->sock),gras_socket_peer_port(g->sock));

  msg_pong_data = NULL;
  if ((errcode=gras_msg_wait(6000,
			     msg_pong_type,&from,(void**)&msg_pong_data))) {
    fprintf(stderr,
	  "Client: Why can't I get my PONG message like everyone else (%s)?\n",
	    gras_error_name(errcode));
    gras_socket_close(&(g->sock));
    return 1;
  }

  fprintf(stderr,"Client: >>>>>>>> Got PONG(%d) got from %s:%d <<<<<<<<\n", 
	  msg_pong_data->dummy,
	  gras_socket_peer_name(from),gras_socket_peer_port(from));

  free(msg_ping_data);  
  free(msg_pong_data);

  gras_sleep(5,0);
  gras_socket_close(&(g->sock));
  fprintf(stderr,"Client: Done.\n");
  return 0;
}
