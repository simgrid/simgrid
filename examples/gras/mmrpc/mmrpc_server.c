/* GridRPC - Fake Grid RPC thingy doing matrix multiplications (as expected)*/

/* Copyright (c) 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#define GRAS_DEFINE_TYPE_EXTERN
#include "xbt/matrix.h"
#include "mmrpc.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(MatMult);


typedef xbt_matrix_t request_t[2];
static int server_cb_request_handler(gras_msg_cb_ctx_t ctx,
                                     void *payload_data)
{

  gras_socket_t expeditor = gras_msg_cb_ctx_from(ctx);

  /* 1. Get the payload into the data variable */
  xbt_matrix_t *request = (xbt_matrix_t *) payload_data;
  xbt_matrix_t result;

  /* 2. Do the computation */
  result = xbt_matrix_double_new_mult(request[0], request[1]);

  /* 3. Send it back as payload of a pong message to the expeditor */
  gras_msg_send(expeditor, "answer", &result);

  /* 4. Cleanups */
  xbt_matrix_free(request[0]);
  xbt_matrix_free(request[1]);
  xbt_matrix_free(result);

  return 0;
}                               /* end_of_server_cb_request_handler */

int server(int argc, char *argv[])
{
  xbt_ex_t e;
  gras_socket_t sock = NULL;
  int port = 4002;

  /* 1. Init the GRAS infrastructure */
  gras_init(&argc, argv);

  /* 2. Get the port I should listen on from the command line, if specified */
  if (argc == 2) {
    port = atoi(argv[1]);
  }

  /* 3. Create my master socket */
  INFO1("Launch server (port=%d)", port);
  TRY {
    sock = gras_socket_server(port);
  }
  CATCH(e) {
    RETHROW0("Unable to establish a server socket: %s");
  }

  /* 4. Register the known messages and payloads. */
  mmrpc_register_messages();

  /* 5. Register my callback */
  gras_cb_register("request", &server_cb_request_handler);

  /* 6. Wait up to 10 minutes for an incomming message to handle */
  gras_msg_handle(600.0);

  /* 7. Free the allocated resources, and shut GRAS down */
  gras_socket_close(sock);
  gras_exit();

  return 0;
}                               /* end_of_server */
