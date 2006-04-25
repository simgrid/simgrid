/* $Id$ */

/* GridRPC - Fake Grid RPC thingy doing matrix multiplications (as expected)*/

/* Copyright (c) 2005 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#define GRAS_DEFINE_TYPE_EXTERN
#include "mmrpc.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(MatMult);

int client(int argc,char *argv[]) {
  xbt_ex_t e; 
  gras_socket_t toserver=NULL; /* peer */

  gras_socket_t from;
  matrix_t request[2], answer;

  int i,j;

  const char *host = "127.0.0.1";
        int   port = 4000;

  /* 1. Init the GRAS's infrastructure */
  gras_init(&argc, argv);
   
  /* 2. Get the server's address. The command line override defaults when specified */
  if (argc == 3) {
    host=argv[1];
    port=atoi(argv[2]);
  } 

  INFO2("Launch client (server on %s:%d)",host,port);
   
  /* 3. Wait for the server startup */
  gras_os_sleep(1);
   
  /* 4. Create a socket to speak to the server */
  TRY {
    toserver=gras_socket_client(host,port);
  } CATCH(e) {
    RETHROW0("Unable to connect to the server: %s");
  }
  INFO2("Connected to %s:%d.",host,port);    


  /* 5. Register the messages (before use) */
  mmrpc_register_messages();

  /* 6. Keep the user informed of what's going on */
  INFO2(">>>>>>>> Connected to server which is on %s:%d <<<<<<<<", 
	gras_socket_peer_name(toserver),gras_socket_peer_port(toserver));

  /* 7. Prepare and send the request to the server */

  request[0].rows=request[0].cols=request[1].rows=request[1].cols=MATSIZE;

  request[0].ctn=xbt_malloc0(sizeof(double)*MATSIZE*MATSIZE);
  request[1].ctn=xbt_malloc0(sizeof(double)*MATSIZE*MATSIZE);

  for (i=0; i<MATSIZE; i++) {
    request[0].ctn[i*MATSIZE+i] = 1;
    for (j=0; j<MATSIZE; j++)
      request[1].ctn[i*MATSIZE+j] = i*MATSIZE+j;
  }
  /*  mat_dump(&request[0],"C:sent0");*/
  /*  mat_dump(&request[1],"C:sent1");*/

  gras_msg_send(toserver, gras_msgtype_by_name("request"), &request);

  free(request[0].ctn);
  free(request[1].ctn);

  INFO2(">>>>>>>> Request sent to %s:%d <<<<<<<<",
	gras_socket_peer_name(toserver),gras_socket_peer_port(toserver));

  /* 8. Wait for the answer from the server, and deal with issues */
  gras_msg_wait(6000,gras_msgtype_by_name("answer"),&from,&answer);

  /*  mat_dump(&answer,"C:answer");*/
  for (i=0; i<MATSIZE*MATSIZE; i++) 
    xbt_assert(answer.ctn[i]==i);

  /* 9. Keep the user informed of what's going on, again */
  INFO2(">>>>>>>> Got answer from %s:%d <<<<<<<<", 
	gras_socket_peer_name(from),gras_socket_peer_port(from));

  /* 10. Free the allocated resources, and shut GRAS down */
  free(answer.ctn);
  gras_socket_close(toserver);
  gras_exit();
  INFO0("Done.");
  return 0;
} /* end_of_client */
