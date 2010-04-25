/* Copyright (c) 2007, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
#include <gras.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(test,"My little example");

void message_declaration(void) {
  gras_msgtype_declare("request", NULL);
  gras_msgtype_declare("grant", NULL);
  gras_msgtype_declare("release", NULL);
}

typedef struct {
   int process_in_CS;
   xbt_dynar_t waiting_queue;
} server_data_t;

int server_request_cb(gras_msg_cb_ctx_t ctx, void *payload) {
  server_data_t *globals=(server_data_t*)gras_userdata_get();
  gras_socket_t s = gras_msg_cb_ctx_from(ctx);
 
  if (globals->process_in_CS) {
     xbt_dynar_push(globals->waiting_queue, &s);
     INFO2("put %s:%d in waiting queue",gras_socket_peer_name(s),gras_socket_peer_port(s));
  } else {     
     globals->process_in_CS = 1;
     INFO2("grant %s:%d since nobody wanted it",gras_socket_peer_name(s),gras_socket_peer_port(s));
     gras_msg_send(s, "grant", NULL);     
  }
  return 0;
} /* end_of_request_callback */

int server_release_cb(gras_msg_cb_ctx_t ctx, void *payload) {
  server_data_t *globals=(server_data_t*)gras_userdata_get();
   
  if (xbt_dynar_length(globals->waiting_queue)) {
     gras_socket_t s;
     xbt_dynar_pop(globals->waiting_queue, &s);
     
     INFO2("grant %s:%d since token released",gras_socket_peer_name(s),gras_socket_peer_port(s));
     gras_msg_send(s, "grant", NULL);
  } else {
     globals->process_in_CS = 0;
  }
   
  return 0;
} /* end_of_release_callback */

int server(int argc, char *argv[]) { 
  gras_socket_t mysock;   /* socket on which I listen */
  server_data_t *globals;
  int i;
  
  gras_init(&argc,argv);
  mysock = gras_socket_server(atoi(argv[1]));

  globals=gras_userdata_new(server_data_t);
  globals->process_in_CS=0;
  globals->waiting_queue=xbt_dynar_new( sizeof(gras_socket_t), NULL /* not closing sockets */);

  message_declaration();   
  gras_cb_register("request",&server_request_cb);
  gras_cb_register("release",&server_release_cb);

  for (i=0; i<20; i++)  /* 5 requests of each process, 2 processes, 2 messages per request */
    gras_msg_handle(-1); 
    
  gras_exit();
  return 0;
} /* end_of_server */

void lock(gras_socket_t toserver) {
   gras_msg_send(toserver,"request",NULL);
   gras_msg_wait(-1, "grant",NULL,NULL);
   INFO0("Granted by server");
} /* end_of_lock */

void unlock(gras_socket_t toserver) {
   INFO0("Release the token");
   gras_msg_send(toserver,"release",NULL);
} /* end_of_unlock */

int client(int argc, char *argv[]) {
  int i;
  gras_socket_t mysock;   /* socket on which I listen */
  gras_socket_t toserver; /* socket used to write to the server */

  gras_init(&argc,argv);

  mysock = gras_socket_server_range(1024, 10000, 0, 0);
  
  VERB1("Client ready; listening on %d", gras_socket_my_port(mysock));
  
  gras_os_sleep(1.5); /* sleep 1 second and half */
  message_declaration();
  toserver = gras_socket_client(argv[1], atoi(argv[2]));

  for (i=0;i<5; i++) {
     gras_os_sleep(0.1);
     lock(toserver);
     gras_os_sleep(0.1);
     unlock(toserver);
  }
  gras_exit();
  return 0;
}
