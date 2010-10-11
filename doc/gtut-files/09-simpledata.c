/* Copyright (c) 2006, 2007, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include <gras.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "My little example");

typedef struct {
  int killed;
} server_data_t;


int server_kill_cb(gras_msg_cb_ctx_t ctx, void *payload)
{
  double delay = *(double *) payload;
  gras_socket_t client = gras_msg_cb_ctx_from(ctx);
  server_data_t *globals = (server_data_t *) gras_userdata_get();

  CRITICAL3("Argh, %s:%d gave me %.2f seconds before suicide!",
            gras_socket_peer_name(client), gras_socket_peer_port(client),
            delay);
  gras_os_sleep(delay);
  CRITICAL0("Bye folks...");


  globals->killed = 1;

  return 0;
}                               /* end_of_kill_callback */

int server_hello_cb(gras_msg_cb_ctx_t ctx, void *payload)
{
  char *msg = *(char **) payload;
  gras_socket_t client = gras_msg_cb_ctx_from(ctx);

  INFO3("Cool, we received a message from %s:%d. Here it is: \"%s\"",
        gras_socket_peer_name(client), gras_socket_peer_port(client), msg);

  return 0;
}                               /* end_of_hello_callback */

void message_declaration(void)
{
  gras_msgtype_declare("kill", gras_datadesc_by_name("double"));
  gras_msgtype_declare("hello", gras_datadesc_by_name("string"));
}


int server(int argc, char *argv[])
{
  gras_socket_t mysock;         /* socket on which I listen */
  server_data_t *globals;

  gras_init(&argc, argv);

  globals = gras_userdata_new(server_data_t *);
  globals->killed = 0;

  message_declaration();
  mysock = gras_socket_server(atoi(argv[1]));

  gras_cb_register("hello", &server_hello_cb);
  gras_cb_register("kill", &server_kill_cb);

  while (!globals->killed) {
    gras_msg_handle(-1);        /* blocking */
  }

  gras_exit();
  return 0;
}

int client(int argc, char *argv[])
{
  gras_socket_t mysock;         /* socket on which I listen */
  gras_socket_t toserver;       /* socket used to write to the server */

  gras_init(&argc, argv);

  message_declaration();
  mysock = gras_socket_server_range(1024, 10000, 0, 0);

  VERB1("Client ready; listening on %d", gras_socket_my_port(mysock));

  gras_os_sleep(1.5);           /* sleep 1 second and half */
  toserver = gras_socket_client(argv[1], atoi(argv[2]));

  char *hello_payload = "Nice to meet you";
  gras_msg_send(toserver, "hello", &hello_payload);
  INFO1("we sent the hello to the server on %s.",
        gras_socket_peer_name(toserver));

  double kill_payload = 0.5;
  gras_msg_send(toserver, "kill", &kill_payload);
  INFO0("Gave the server more 0.5 second to live");

  gras_exit();
  return 0;
}
