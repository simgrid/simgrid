/* $Id$ */

/* ping - ping/pong demo of GRAS features                                   */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "gras.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(Ping,"Messages specific to this example");

/* **********************************************************************
 * Comon code
 * **********************************************************************/

typedef struct {
  int dummy;
} msg_ping_t;

/* Function prototypes */
void register_messages(void);

/* Code */
void register_messages(void) {

  gras_msgtype_declare("ping", gras_datadesc_by_name("int"));
  gras_msgtype_declare("pong", gras_datadesc_by_name("int"));
}

/* **********************************************************************
 * Server code
 * **********************************************************************/

/* Global private data */
typedef struct {
  gras_socket_t sock;
  int endcondition;
} server_data_t;

/* Function prototypes */
int server_cb_ping_handler(gras_socket_t  expeditor,
			   void          *payload_data);
int server (int argc,char *argv[]);


int server_cb_ping_handler(gras_socket_t  expeditor,
			   void          *payload_data) {
			     
  xbt_error_t errcode;
  int msg=*(int*)payload_data;
  gras_msgtype_t *pong_t=NULL;

  server_data_t *g=(server_data_t*)gras_userdata_get();

  g->endcondition = 0;
  INFO3("SERVER: >>>>>>>> Got message PING(%d) from %s:%d <<<<<<<<", 
	msg, 
 	gras_socket_peer_name(expeditor),
	gras_socket_peer_port(expeditor));
  
  msg = 4321;
  errcode = gras_msg_send(expeditor, gras_msgtype_by_name("pong"), &msg);

  if (errcode != no_error) {
    ERROR1("SERVER: Unable answer with PONG: %s\n", xbt_error_name(errcode));
    gras_socket_close(g->sock);
    return 1;
  }

  INFO0("SERVER: >>>>>>>> Answed with PONG(4321) <<<<<<<<");
  g->endcondition = 1;
  gras_socket_close(expeditor);
  return 1;
}

int server (int argc,char *argv[]) {
  xbt_error_t errcode;
  server_data_t *g;
  gras_msgtype_t *ping_msg=NULL;

  int port = 4000;
  
  gras_init(&argc,argv, NULL);
  g=gras_userdata_new(server_data_t);
   
  if (argc == 2) {
    port=atoi(argv[1]);
  }

  INFO1("Launch server (port=%d)", port);

  if ((errcode=gras_socket_server(port,&(g->sock)))) { 
    CRITICAL1("Error %s encountered while opening the server socket",
	      xbt_error_name(errcode));
    return 1;
  }

  register_messages();
  register_messages();
  gras_cb_register(gras_msgtype_by_name("ping"),&server_cb_ping_handler);

  INFO1("SERVER: >>>>>>>> Listening on port %d <<<<<<<<",
	gras_socket_my_port(g->sock));
  g->endcondition=0;

  errcode = gras_msg_handle(600.0);
  if (errcode != no_error)
    return errcode;
  if (g->endcondition)
  
  if (!gras_if_RL())
    gras_os_sleep(1,0);

  gras_socket_close(g->sock);
  free(g);
  gras_exit();
  INFO0("SERVER: Done.");
  return no_error;
}

/* **********************************************************************
 * Client code
 * **********************************************************************/

/* Global private data */
typedef struct {
  gras_socket_t sock;
} client_data_t;

/* Function prototypes */
int client (int argc,char *argv[]);

int client(int argc,char *argv[]) {
  xbt_error_t errcode;
  client_data_t *g;

  gras_socket_t from;
  int ping, pong;

  const char *host = "127.0.0.1";
        int   port = 4000;

  gras_init(&argc, argv, NULL);
  g=gras_userdata_new(client_data_t);
   
  if (argc == 3) {
    host=argv[1];
    port=atoi(argv[2]);
  } 

  INFO2("Launch client (server on %s:%d)",host,port);
  gras_os_sleep(1,0); /* Wait for the server startup */
  if ((errcode=gras_socket_client(host,port,&(g->sock)))) {
    ERROR1("Client: Unable to connect to the server. Got %s",
	   xbt_error_name(errcode));
    return 1;
  }
  INFO2("Client: Connected to %s:%d.",host,port);    


  register_messages();

  INFO2("Client: >>>>>>>> Connected to server which is on %s:%d <<<<<<<<", 
	gras_socket_peer_name(g->sock),gras_socket_peer_port(g->sock));

  ping = 1234;
  errcode = gras_msg_send(g->sock, gras_msgtype_by_name("ping"), &ping);
  if (errcode != no_error) {
    fprintf(stderr, "Client: Unable send PING to server (%s)\n",
	    xbt_error_name(errcode));
    gras_socket_close(g->sock);
    return 1;
  }
  INFO3("Client: >>>>>>>> Message PING(%d) sent to %s:%d <<<<<<<<",
	ping,
	gras_socket_peer_name(g->sock),gras_socket_peer_port(g->sock));

  if ((errcode=gras_msg_wait(6000,gras_msgtype_by_name("pong"),
			     &from,&pong))) {
    ERROR1("Client: Why can't I get my PONG message like everyone else (%s)?",
	   xbt_error_name(errcode));
    gras_socket_close(g->sock);
    return 1;
  }

  INFO3("Client: >>>>>>>> Got PONG(%d) got from %s:%d <<<<<<<<", 
	pong,
	gras_socket_peer_name(from),gras_socket_peer_port(from));

  gras_socket_close(g->sock);
  free(g);
  gras_exit();
  INFO0("Client: Done.");
  return 0;
}
