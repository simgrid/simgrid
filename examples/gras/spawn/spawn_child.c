/* spawn - demo of the gras_agent_spawn function                            */

/* Copyright (c) 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "spawn.h"
XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(Spawn);

int child(int argc, char *argv[])
{
  xbt_ex_t e;
  gras_socket_t dady = NULL;    /* peer */

  gras_socket_t from;
  int ping, pong;

  const char *host = gras_os_myname();
  int port = 4000;

  /* 1. Init the GRAS's infrastructure */
  gras_init(&argc, argv);

  /* 2. Get the server's address. The command line override defaults when specified */
  if (argc == 2) {
    port = atoi(argv[1]);
  }

  gras_socket_server_range(4000, 5000, 0, 0);

  /* 3. Connect back to my father */
  TRY {
    dady = gras_socket_client(host, port);
  }
  CATCH(e) {
    RETHROW0("Unable to connect to my dady: %s");
  }
  INFO4("I (%s:%d) have found my dady on %s:%d.",
        gras_os_myname(), gras_os_myport(), host, port);


  /* 4. Register the messages. */
  spawn_register_messages();

  /* 5. Ping my dady */
  ping = 1234;
  TRY {
    gras_msg_send(dady, "ping", &ping);
  }
  CATCH(e) {
    gras_socket_close(dady);
    RETHROW0("Failed to ping my dady: %s");
  }

  /* 6. Wait for the answer from the server, and deal with issues */
  TRY {
    gras_msg_wait(6000, "pong", &from, &pong);
  }
  CATCH(e) {
    gras_socket_close(dady);
    RETHROW0("Dad don't want to speak with me! : %s");
  }
  INFO2("Pinged dad with %d, he answered with %d; leaving now.", ping, pong);

  /* 7. Free the allocated resources, and shut GRAS down */
  gras_socket_close(dady);
  gras_exit();
  return 0;
}                               /* end_of_child */
