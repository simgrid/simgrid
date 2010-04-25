/* ping - ping/pong demo of GRAS features                                   */

/* Copyright (c) 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "ping.h"
XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(Ping);

int client(int argc, char *argv[])
{
  xbt_ex_t e;
  gras_socket_t toserver = NULL;        /* peer */
  int connected = 0;

  gras_socket_t from;
  int ping, pong;

  const char *host = "127.0.0.1";
  int port = 4000;

  /* 1. Init the GRAS's infrastructure */
  gras_init(&argc, argv);

  /* 2. Get the server's address. The command line override defaults when specified */
  if (argc == 3) {
    host = argv[1];
    port = atoi(argv[2]);
  }

  INFO2("Launch client (server on %s:%d)", host, port);

  /* 3. Create a socket to speak to the server */
  while (!connected) {
    TRY {
      toserver = gras_socket_client(host, port);
      connected = 1;
    }
    CATCH(e) {
      if (e.category != system_error)
        /* dunno what happened, let the exception go through */
        RETHROW0("Unable to connect to the server: %s");
      xbt_ex_free(e);
      gras_os_sleep(0.05);
    }
  }

  INFO2("Connected to %s:%d.", host, port);

  /* 4. Register the messages.
     See, it doesn't have to be done completely at the beginning, only before use */
  ping_register_messages();

  /* 5. Keep the user informed of what's going on */
  INFO2(">>>>>>>> Connected to server which is on %s:%d <<<<<<<<",
        gras_socket_peer_name(toserver), gras_socket_peer_port(toserver));

  /* 6. Prepare and send the ping message to the server */
  ping = 1234;
  TRY {
    gras_msg_send(toserver, "ping", &ping);
  }
  CATCH(e) {
    gras_socket_close(toserver);
    RETHROW0("Failed to send PING to server: %s");
  }
  INFO3(">>>>>>>> Message PING(%d) sent to %s:%d <<<<<<<<",
        ping,
        gras_socket_peer_name(toserver), gras_socket_peer_port(toserver));

  /* 7. Wait for the answer from the server, and deal with issues */
  TRY {
    gras_msg_wait(6000, "pong", &from, &pong);
  }
  CATCH(e) {
    gras_socket_close(toserver);
    RETHROW0("Why can't I get my PONG message like everyone else: %s");
  }

  /* 8. Keep the user informed of what's going on, again */
  INFO3(">>>>>>>> Got PONG(%d) from %s:%d <<<<<<<<",
        pong, gras_socket_peer_name(from), gras_socket_peer_port(from));

  /* 9. Free the allocated resources, and shut GRAS down */
  gras_socket_close(toserver);
  INFO0("Done.");
  gras_exit();
  return 0;
}                               /* end_of_client */
