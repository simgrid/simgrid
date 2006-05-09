/* $Id$ */

/* ALLTOALL - alltoall of GRAS features                         */

/* Copyright (c) 2006 Ahmed Harbaoui. All rights reserved.      */

 /* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras.h"
#include "xbt/ex.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(alltoall,"Messages specific to this example");

/* register data which may be sent (common to client and server) */
static void register_messages(void) {
  gras_msgtype_declare("data", gras_datadesc_by_name("int"));
}

/* Function prototypes */
int node (int argc,char *argv[]);


/* **********************************************************************
 * node code
 * **********************************************************************/

/* Global private data */
typedef struct {
  gras_socket_t sock;
  int done;
} node_data_t;

static void free_host(void *d){
  xbt_host_t h=*(xbt_host_t*)d;
  free(h->name);
  free(h);
}

static void kill_buddy(char *name,int port){
	gras_socket_t sock=gras_socket_client(name,port);
	gras_msg_send(sock,gras_msgtype_by_name("kill"),NULL);
	gras_socket_close(sock);
}

static void kill_buddy_dynar(void *b) {
	xbt_host_t buddy=*(xbt_host_t*)b;
	kill_buddy(buddy->name,buddy->port);
}

static int node_cb_data_handler(gras_msg_cb_ctx_t ctx,
   			          void          *payload_data) {

  /* Get the payload into the msg variable */
  int data=*(int*)payload_data;

  gras_socket_t expeditor = gras_msg_cb_ctx_from(ctx);

  /* Retrieve the server's state (globals) */

  node_data_t *globals=(node_data_t*)gras_userdata_get();
  globals->done = 0;

  /* Log which client connected */
  INFO3(">>>>>>>> Got Data(%d) from %s:%d <<<<<<<<", 
	data, 
	gras_socket_peer_name(expeditor), gras_socket_peer_port(expeditor));
 
  /* Set the done boolean to true (and make sure the server stops after receiving it). */
  globals->done = 1;
  
  /* Make sure we don't leak sockets */
  //gras_socket_close(expeditor);
   
  /* Tell GRAS that we consummed this message */
  return 1;
} /* end_of_server_cb_ping_handler */

int node (int argc,char *argv[]) {

  xbt_ex_t e;
	
  int port,nb_hosts,data,
  i,done;
 
  xbt_host_t h1;
  
  gras_socket_t peer;  /* socket to node */
 
  node_data_t *globals;
 
  /* xbt_dynar for hosts */
  xbt_dynar_t hosts = xbt_dynar_new(sizeof(xbt_host_t),&free_host);
 
  /* Init the GRAS infrastructure and declare my globals */
  gras_init(&argc,argv);
 
  globals=gras_userdata_new(node_data_t *);

  /* Get the port I should listen on from the command line, if specified */
  if (argc > 2) {
    port=atoi(argv[1]);
  }

  /* Get the node location from argc/argv */
  for (i=2; i<argc; i++){
    xbt_host_t host=xbt_new(s_xbt_host_t,1);
    host->name=strdup(argv[i]);
    host->port=atoi(argv[1]);
    INFO2("New node : %s:%d",host->name,host->port);
    xbt_dynar_push(hosts,&host);
  }
  nb_hosts = xbt_dynar_length(hosts);

  INFO1("Launch current node (port=%d)", port);

  /* Create my master socket */
  globals->sock = gras_socket_server(port);

  /* Register the known messages */
  register_messages();
  register_messages();

  /* 3. Wait for others nodesthe startup */
  gras_os_sleep(1);
  

  /* Register my callback */
  gras_cb_register(gras_msgtype_by_name("data"),&node_cb_data_handler);

  INFO1(">>>>>>>> Listening on port %d <<<<<<<<", gras_socket_my_port(globals->sock));
  globals->done=0;
 
  data =1000;
  xbt_dynar_foreach(hosts,i,h1) {
	  peer = gras_socket_client(h1->name,h1->port);
   done=0;	
    while (!done){
      TRY {
        gras_msg_handle(0);
      }CATCH(e){
	if (e.category != timeout_error)
	RETHROW;
	xbt_ex_free(e);
	done = 1;
      }
    }
    
    gras_msg_send(peer,gras_msgtype_by_name("data"),&data);
    INFO3(">>>>>>>> Send Data (%d) from %s to %s <<<<<<<<", 
	  data,argv[0],h1->name);
  }

  if (!globals->done)
     WARN0("An error occured, the done was not set by the callback");

 /* Free the allocated resources, and shut GRAS down */
  gras_socket_close(globals->sock);
  free(globals);
  gras_exit();

  //xbt_dynar_map(hosts,kill_buddy_dynar);
  //xbt_dynar_free(&hosts);
  
  INFO0("Done.");
  return 0;
} /* end_of_node */