/* Copyright (c) 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include <gras.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(test,"My little example");

/* *********************** Server *********************** */
typedef struct {
   int killed;
} server_data_t;   

int server_kill_cb(gras_msg_cb_ctx_t ctx, void *payload) {
  gras_socket_t client = gras_msg_cb_ctx_from(ctx);
  server_data_t *globals=(server_data_t*)gras_userdata_get();
   
  CRITICAL2("Argh, killed by %s:%d! Bye folks...",
	gras_socket_peer_name(client), gras_socket_peer_port(client));
  
  globals->killed = 1;
   
  return 0;
} /* end_of_kill_callback */

int server_hello_cb(gras_msg_cb_ctx_t ctx, void *payload) {
  gras_socket_t client = gras_msg_cb_ctx_from(ctx);

  INFO2("Cool, we received the message from %s:%d.",
	gras_socket_peer_name(client), gras_socket_peer_port(client));
  
  return 0;
} /* end_of_hello_callback */

int server(int argc, char *argv[]) {
  gras_socket_t mysock;   /* socket on which I listen */
  server_data_t *globals;
  
  gras_init(&argc,argv);

  globals=gras_userdata_new(server_data_t*);
  globals->killed=0;

  gras_msgtype_declare("hello", NULL);
  gras_msgtype_declare("kill", NULL);
  mysock = gras_socket_server(atoi(argv[1]));
   
  gras_cb_register("hello",&server_hello_cb);   
  gras_cb_register("kill",&server_kill_cb);

  while (!globals->killed) {
     gras_msg_handle(60);
  }
    
  gras_exit();
  return 0;
}

/* *********************** Client *********************** */
/* client_data */
typedef struct {
   gras_socket_t toserver;
   int done;
} client_data_t;
 

void client_do_hello(void) {
  client_data_t *globals=(client_data_t*)gras_userdata_get();
   
  gras_msg_send(globals->toserver,"hello", NULL);
  INFO0("Hello sent to server");
} /* end_of_client_do_hello */

void client_do_stop(void) {
  client_data_t *globals=(client_data_t*)gras_userdata_get();
   
  gras_msg_send(globals->toserver,"kill", NULL);
  INFO0("Kill sent to server");
   
  gras_timer_cancel_repeat(0.5,client_do_hello);
 
  globals->done = 1;
  INFO0("Break the client's while loop");
} /* end_of_client_do_stop */

int client(int argc, char *argv[]) {
  gras_socket_t mysock;   /* socket on which I listen */
  client_data_t *globals;

  gras_init(&argc,argv);

  gras_msgtype_declare("hello", NULL);
  gras_msgtype_declare("kill", NULL);
  mysock = gras_socket_server_range(1024, 10000, 0, 0);
  
  VERB1("Client ready; listening on %d", gras_socket_my_port(mysock));
  
  globals=gras_userdata_new(client_data_t*);
  globals->done = 0;
  
  gras_os_sleep(1.5); /* sleep 1 second and half */
  globals->toserver = gras_socket_client(argv[1], atoi(argv[2]));
   
  INFO0("Programming the repetitive action with a frequency of 0.5 sec");
  gras_timer_repeat(0.5,client_do_hello);
  
  INFO0("Programming the delayed action in 5 secs");
  gras_timer_delay(5,client_do_stop);
  
  while (!globals->done) {
     gras_msg_handle(60);
  }

  gras_exit();
  return 0;
}

