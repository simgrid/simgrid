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
  gras_socket_t client = gras_msg_cb_ctx_from(ctx);
  server_data_t *globals = (server_data_t *) gras_userdata_get();

  CRITICAL2("Argh, killed by %s:%d! Bye folks, I'm out of here...",
            gras_socket_peer_name(client), gras_socket_peer_port(client));

  globals->killed = 1;

  return 0;
}                               /* end_of_kill_callback */

int server(int argc, char *argv[])
{
  gras_socket_t mysock;         /* socket on which I listen */
  server_data_t *globals;

  gras_init(&argc, argv);

  globals = gras_userdata_new(server_data_t *);
  globals->killed = 0;

  gras_msgtype_declare("kill", NULL);
  gras_cb_register("kill", &server_kill_cb);

  if (argc > 1 && !strcmp(argv[1], "--cheat")) {
    mysock = gras_socket_server(9999);
    INFO0("Hi! hi! I'm not in the search range, but in 9999...");
  } else {
    mysock = gras_socket_server((rand() % 10) + 3000);
    INFO1("Ok, I'm hidden on port %d. Hope for the best.",
          gras_socket_my_port(mysock));
  }

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
  int found;                    /* whether we found peer */
  int port;                     /* where we think that the server is */
  xbt_ex_t e;

  gras_init(&argc, argv);

  gras_msgtype_declare("kill", NULL);
  mysock = gras_socket_server_range(1024, 10000, 0, 0);

  VERB0("Run little server, run. I'll get you. (sleep 1.5 sec)");
  gras_os_sleep(1.5);

  for (port = 3000, found = 0; port < 3010 && !found; port++) {
    TRY {
      toserver = gras_socket_client(argv[1], port);
      gras_msg_send(toserver, "kill", NULL);
      gras_socket_close(toserver);
      found = 1;
      INFO1("Yeah! I found the server on %d! It's eradicated by now.",
            port);
    }
    CATCH(e) {
      xbt_ex_free(e);
    }
    if (!found)
      INFO1("Damn, the server is not on %d", port);
  }                             /* end_of_loop */

  if (!found)
    THROW0(not_found_error, 0,
           "Damn, I failed to find the server! I cannot survive this humilliation.");


  gras_exit();
  return 0;
}
