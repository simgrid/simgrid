/* ping - ping/pong demo of GRAS features                                   */

/* Copyright (c) 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "ping.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(Ping);

/* Global private data */
typedef struct {
  gras_socket_t sock;
  int endcondition;
} server_data_t;


static int server_cb_ping_handler(gras_msg_cb_ctx_t ctx, void *payload)
{

  xbt_ex_t e;
  /* 1. Get the payload into the msg variable, and retrieve my caller */
  int msg = *(int *) payload;
  gras_socket_t expeditor = gras_msg_cb_ctx_from(ctx);

  /* 2. Retrieve the server's state (globals) */

  server_data_t *globals = (server_data_t *) gras_userdata_get();
  globals->endcondition = 0;

  /* 3. Log which client connected */
  INFO3(">>>>>>>> Got message PING(%d) from %s:%d <<<<<<<<",
        msg,
        gras_socket_peer_name(expeditor), gras_socket_peer_port(expeditor));

  /* 4. Change the value of the msg variable */
  msg = 4321;
  /* 5. Send it back as payload of a pong message to the expeditor */
  TRY {
    gras_msg_send(expeditor, "pong", &msg);

    /* 6. Deal with errors: add some details to the exception */
  } CATCH(e) {
    gras_socket_close(globals->sock);
    RETHROW0("Unable answer with PONG: %s");
  }

  INFO0(">>>>>>>> Answered with PONG(4321) <<<<<<<<");

  /* 7. Set the endcondition boolean to true (and make sure the server stops after receiving it). */
  globals->endcondition = 1;

  /* 8. Tell GRAS that we consummed this message */
  return 0;
}                               /* end_of_server_cb_ping_handler */

int server(int argc, char *argv[])
{
  server_data_t *globals;

  int port = 4000;

  /* 1. Init the GRAS infrastructure and declare my globals */
  gras_init(&argc, argv);
  globals = gras_userdata_new(server_data_t);

  /* 2. Get the port I should listen on from the command line, if specified */
  if (argc == 2) {
    port = atoi(argv[1]);
  }

  INFO1("Launch server (port=%d)", port);

  /* 3. Create my master socket */
  globals->sock = gras_socket_server(port);

  /* 4. Register the known messages. This function is called twice here, but it's because
     this file also acts as regression test, no need to do so yourself of course. */
  ping_register_messages();
  ping_register_messages();     /* just to make sure it works ;) */

  /* 5. Register my callback */
  gras_cb_register("ping", &server_cb_ping_handler);

  INFO1(">>>>>>>> Listening on port %d <<<<<<<<",
        gras_socket_my_port(globals->sock));
  globals->endcondition = 0;

  /* 6. Wait up to 10 minutes for an incomming message to handle */
  gras_msg_handle(10.0);

  /* 7. Housekeeping */
  if (!globals->endcondition)
    WARN0("An error occured, the endcondition was not set by the callback");

  /* 8. Free the allocated resources, and shut GRAS down */
  gras_socket_close(globals->sock);
  free(globals);
  INFO0("Done.");
  gras_exit();

  return 0;
}                               /* end_of_server */
