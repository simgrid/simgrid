/* spawn - demo of the gras_agent_spawn function                            */

/* Copyright (c) 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "spawn.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(Spawn);

/* Global private data */
typedef struct {
  gras_socket_t sock;
  int msg_got;
} father_data_t;


static int father_cb_ping_handler(gras_msg_cb_ctx_t ctx, void *payload)
{

  xbt_ex_t e;
  /* 1. Get the payload into the msg variable, and retrieve my caller */
  int msg = *(int *) payload;
  gras_socket_t expeditor = gras_msg_cb_ctx_from(ctx);

  /* 2. Retrieve the father's state (globals) */
  father_data_t *globals = (father_data_t *) gras_userdata_get();
  globals->msg_got++;

  /* 3. Log which client connected */
  INFO3("Kid %s:%d pinged me with %d",
        gras_socket_peer_name(expeditor), gras_socket_peer_port(expeditor),
        msg);

  /* 4. Change the value of the msg variable */
  msg = 4321;
  /* 5. Send it back as payload of a pong message to the expeditor */
  TRY {
    gras_msg_send(expeditor, "pong", &msg);

    /* 6. Deal with errors: add some details to the exception */
  } CATCH(e) {
    gras_socket_close(globals->sock);
    RETHROW0("Unable to answer to my poor child: %s");
  }
  INFO2("Answered to %s:%d's request",
        gras_socket_peer_name(expeditor),
        gras_socket_peer_port(expeditor));

  /* 7. Tell GRAS that we consummed this message */
  return 0;
}                               /* end_of_father_cb_ping_handler */

int father(int argc, char *argv[])
{
  father_data_t *globals;

  int port = 4000;
  int child_amount = 5;
  char **child_args;
  int i;

  /* 1. Init the GRAS infrastructure and declare my globals */
  gras_init(&argc, argv);
  globals = gras_userdata_new(father_data_t);

  /* 2. Get args from the command line, if specified */
  if (argc == 2) {
    port = atoi(argv[1]);
    child_amount = atoi(argv[2]);
  }

  /* 3. Initialize my globals (mainly create my master socket) */
  globals->sock = gras_socket_server(port);
  globals->msg_got = 0;

  /* 4. Register the known messages. */
  spawn_register_messages();

  /* 5. Register my callback */
  gras_cb_register("ping", &father_cb_ping_handler);

  /* 6. Spawn the kids */
  INFO0("Spawn the kids");
  for (i = 0; i < child_amount; i++) {
    child_args = xbt_new0(char *, 3);
    child_args[0] = xbt_strdup("child");
    child_args[1] = bprintf("%d", port);
    child_args[2] = NULL;
    gras_agent_spawn("child", NULL, child, 2, child_args, NULL);
  }

  /* 7. Wait to be contacted be the kids */
  while (globals->msg_got < child_amount)
    gras_msg_handle(60.0);
  INFO0("All kids gone. Leave now.");

  /* 8. Free the allocated resources, and shut GRAS down */
  gras_socket_close(globals->sock);
  free(globals);
  gras_exit();

  return 0;
}                               /* end_of_father */
