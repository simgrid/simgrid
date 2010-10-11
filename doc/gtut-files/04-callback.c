/* Copyright (c) 2006, 2007, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <gras.h>

int server_hello_cb(gras_msg_cb_ctx_t ctx, void *payload)
{
  gras_socket_t client = gras_msg_cb_ctx_from(ctx);

  fprintf(stderr, "Cool, we received the message from %s:%d.\n",
          gras_socket_peer_name(client), gras_socket_peer_port(client));

  return 0;
}                               /* end_of_callback */

int server(int argc, char *argv[])
{
  gras_socket_t mysock;         /* socket on which I listen */

  gras_init(&argc, argv);

  gras_msgtype_declare("hello", NULL);
  mysock = gras_socket_server(atoi(argv[1]));

  gras_cb_register("hello", &server_hello_cb);
  gras_msg_handle(60);

  gras_exit();
  return 0;
}

int client(int argc, char *argv[])
{
  gras_socket_t mysock;         /* socket on which I listen */
  gras_socket_t toserver;       /* socket used to write to the server */

  gras_init(&argc, argv);

  gras_msgtype_declare("hello", NULL);
  mysock = gras_socket_server_range(1024, 10000, 0, 0);

  fprintf(stderr, "Client ready; listening on %d\n",
          gras_socket_my_port(mysock));

  gras_os_sleep(1.5);           /* sleep 1 second and half */
  toserver = gras_socket_client(argv[1], atoi(argv[2]));

  gras_msg_send(toserver, "hello", NULL);
  fprintf(stderr, "That's it, we sent the data to the server on %s\n",
          gras_socket_peer_name(toserver));

  gras_exit();
  return 0;
}
