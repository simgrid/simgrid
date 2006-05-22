/* $Id$ */

/* GridRPC - Fake Grid RPC thingy doing matrix multiplications (as expected)*/

/* Copyright (c) 2005 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#define GRAS_DEFINE_TYPE_EXTERN
#include "mmrpc.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(MatMult);


static int server_cb_request_handler(gras_msg_cb_ctx_t ctx, void *payload_data) {
  gras_socket_t expeditor=gras_msg_cb_ctx_from(ctx);
			     
  /* 1. Get the payload into the data variable */
  matrix_t *data=(matrix_t*)payload_data;
  matrix_t result;
  int i,j,k;
   
  /* 2. Make some room to return the result */
  result.lines = data[0].lines;
  result.rows = data[1].rows;
  result.ctn = xbt_malloc0(sizeof(double) * result.lines * result.rows);

  /* 3. Do the computation */
  for (i=0; i<result.lines; i++) 
    for (j=0; j<result.rows; j++) 
      for (k=0; k<data[1].lines; k++) 
	result.ctn[i*result.rows + j] +=  data[0].ctn[i*result.rows +k] *data[1].ctn[k*result.rows +j];

  /* 4. Send it back as payload of a pong message to the expeditor */
  gras_msg_send(expeditor, gras_msgtype_by_name("answer"), &result);

  /* 5. Cleanups */
  free(data[0].ctn); 
  free(data[1].ctn);
  free(result.ctn);
  gras_socket_close(expeditor);
   
  return 1;
} /* end_of_server_cb_request_handler */

int server (int argc,char *argv[]) {
  xbt_ex_t e; 
  gras_socket_t sock=NULL;
  int port = 4000;
  
  /* 1. Init the GRAS infrastructure */
  gras_init(&argc,argv);
   
  /* 2. Get the port I should listen on from the command line, if specified */
  if (argc == 2) {
    port=atoi(argv[1]);
  }

  /* 3. Create my master socket */
  INFO1("Launch server (port=%d)", port);
  TRY {
    sock = gras_socket_server(port);
  } CATCH(e) {
    RETHROW0("Unable to establish a server socket: %s");
  }

  /* 4. Register the known messages and payloads. */
  mmrpc_register_messages();
   
  /* 5. Register my callback */
  gras_cb_register(gras_msgtype_by_name("request"),&server_cb_request_handler);

  /* 6. Wait up to 10 minutes for an incomming message to handle */
  gras_msg_handle(600.0);
   
  /* 7. Free the allocated resources, and shut GRAS down */
  gras_socket_close(sock);
  gras_exit();
   
  INFO0("Done.");
  return 0;
} /* end_of_server */
