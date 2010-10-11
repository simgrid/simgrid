/* GridRPC - Fake Grid RPC thingy doing matrix multiplications (as expected)*/

/* Copyright (c) 2005, 2006, 2007, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#define GRAS_DEFINE_TYPE_EXTERN
#include "xbt/matrix.h"
#include "mmrpc.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(MatMult, "Messages specific to this example");

/* register messages which may be sent and their payload
   (common to client and server) */
void mmrpc_register_messages(void)
{
  gras_datadesc_type_t matrix_type, request_type;

  matrix_type = gras_datadesc_matrix(gras_datadesc_by_name("double"),
                                     NULL);
  request_type =
      gras_datadesc_array_fixed("s_matrix_t(double)[2]", matrix_type, 2);

  gras_msgtype_declare("answer", matrix_type);
  gras_msgtype_declare("request", request_type);
}

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
  gras_socket_close(expeditor);

  return 0;
}                               /* end_of_server_cb_request_handler */

int server(int argc, char *argv[])
{
  xbt_ex_t e;
  gras_socket_t sock = NULL;
  int port = 4000;

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

  INFO0("Done.");
  return 0;
}                               /* end_of_server */


int client(int argc, char *argv[])
{
  xbt_ex_t e;
  gras_socket_t toserver = NULL;        /* peer */

  gras_socket_t from;
  xbt_matrix_t request[2], answer;

  int i, j;

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

  /* 3. Wait for the server startup */
  gras_os_sleep(1);

  /* 4. Create a socket to speak to the server */
  TRY {
    toserver = gras_socket_client(host, port);
  }
  CATCH(e) {
    RETHROW0("Unable to connect to the server: %s");
  }
  INFO2("Connected to %s:%d.", host, port);


  /* 5. Register the messages (before use) */
  mmrpc_register_messages();

  /* 6. Keep the user informed of what's going on */
  INFO2(">>>>>>>> Connected to server which is on %s:%d <<<<<<<<",
        gras_socket_peer_name(toserver), gras_socket_peer_port(toserver));

  /* 7. Prepare and send the request to the server */

  request[0] = xbt_matrix_double_new_id(MATSIZE, MATSIZE);
  request[1] = xbt_matrix_double_new_rand(MATSIZE, MATSIZE);

  /*
     xbt_matrix_dump(request[0],"C:sent0",0,xbt_matrix_dump_display_double);
     xbt_matrix_dump(request[1],"C:sent1",0,xbt_matrix_dump_display_double);
   */

  gras_msg_send(toserver, "request", &request);

  xbt_matrix_free(request[0]);

  INFO2(">>>>>>>> Request sent to %s:%d <<<<<<<<",
        gras_socket_peer_name(toserver), gras_socket_peer_port(toserver));

  /* 8. Wait for the answer from the server, and deal with issues */
  gras_msg_wait(6000, "answer", &from, &answer);

  /*
     xbt_matrix_dump(answer,"C:answer",0,xbt_matrix_dump_display_double);
   */
  for (i = 0; i < MATSIZE; i++)
    for (j = 0; i < MATSIZE; i++)
      xbt_assert4(xbt_matrix_get_as(answer, i, j, double) ==
                  xbt_matrix_get_as(request[1], i, j, double),
                  "Answer does not match expectations. Found %f at cell %d,%d instead of %f",
                  xbt_matrix_get_as(answer, i, j, double), i, j,
                  xbt_matrix_get_as(request[1], i, j, double));

  /* 9. Keep the user informed of what's going on, again */
  INFO2(">>>>>>>> Got answer from %s:%d (values are right) <<<<<<<<",
        gras_socket_peer_name(from), gras_socket_peer_port(from));

  /* 10. Free the allocated resources, and shut GRAS down */
  xbt_matrix_free(request[1]);
  xbt_matrix_free(answer);
  gras_socket_close(toserver);
  gras_exit();
  INFO0("Done.");
  return 0;
}                               /* end_of_client */
