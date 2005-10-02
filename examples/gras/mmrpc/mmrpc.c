/* $Id$ */

/* GridRPC - Fake Grid RPC thingy doing matrix multiplications (as expected)*/

/* Copyright (c) 2005 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras.h"

#define MATSIZE 128

XBT_LOG_NEW_DEFAULT_CATEGORY(MatMult,"Messages specific to this example");

GRAS_DEFINE_TYPE(s_matrix,
struct s_matrix {
  int rows;
  int cols;
  double *ctn GRAS_ANNOTE(size, rows*cols);
};);
typedef struct s_matrix matrix_t;

static void mat_dump(matrix_t *mat, const char* name) {
  int i,j;

  printf(">>> Matrix %s dump (%d x %d)\n",name,mat->rows,mat->cols);
  for (i=0; i<mat->rows; i++) {
    printf("  ");
    for (j=0; j<mat->cols; j++)
      printf(" %.2f",mat->ctn[i*mat->cols + j]);
    printf("\n");
  }
  printf("<<< end of matrix %s dump\n",name);
}

/* register messages which may be sent and their payload
   (common to client and server) */
static void register_messages(void) {
  gras_datadesc_type_t matrix_type, request_type;

  matrix_type=gras_datadesc_by_symbol(s_matrix);
  request_type=gras_datadesc_array_fixed("matrix_t[2]",matrix_type,2);
  
  gras_msgtype_declare("answer", matrix_type);
  gras_msgtype_declare("request", request_type);
}

/* Function prototypes */
int server (int argc,char *argv[]);
int client (int argc,char *argv[]);

/* **********************************************************************
 * Server code
 * **********************************************************************/

static int server_cb_request_handler(gras_socket_t expeditor, void *payload_data) {
			     
  /* 1. Get the payload into the data variable */
  matrix_t *data=(matrix_t*)payload_data;
  matrix_t result;
  int i,j,k;
   
  /* 2. Make some room to return the result */
  result.rows = data[0].rows;
  result.cols = data[1].cols;
  result.ctn = xbt_malloc0(sizeof(double) * result.rows * result.cols);

  /* 3. Do the computation */
  for (i=0; i<result.rows; i++) 
    for (j=0; j<result.cols; j++) 
      for (k=0; k<data[1].rows; k++) 
	result.ctn[i*result.cols + j] +=  data[0].ctn[i*result.cols +k] *data[1].ctn[k*result.cols +j];

  /* 4. Send it back as payload of a pong message to the expeditor */
  gras_msg_send(expeditor, gras_msgtype_by_name("answer"), &result);

  /* 5. Cleanups */
  free(data[0].ctn); 
  free(data[1].ctn);
  free(data);
  free(result.ctn);
  gras_socket_close(expeditor);
   
  return 1;
} /* end_of_server_cb_request_handler */

int server (int argc,char *argv[]) {
  xbt_ex_t e; 
  gras_socket_t sock;
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
  register_messages();
   
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

/* **********************************************************************
 * Client code
 * **********************************************************************/

/* Function prototypes */

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
  register_messages();

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
  //  mat_dump(&request[0],"C:sent0");
  //  mat_dump(&request[1],"C:sent1");

  gras_msg_send(toserver, gras_msgtype_by_name("request"), &request);

  free(request[0].ctn);
  free(request[1].ctn);

  INFO2(">>>>>>>> Request sent to %s:%d <<<<<<<<",
	gras_socket_peer_name(toserver),gras_socket_peer_port(toserver));

  /* 8. Wait for the answer from the server, and deal with issues */
  gras_msg_wait(6000,gras_msgtype_by_name("answer"),&from,&answer);

  //  mat_dump(&answer,"C:answer");
  for (i=0; i<MATSIZE*MATSIZE; i++) 
    xbt_assert(answer.ctn[i]==i);

  /* 9. Keep the user informed of what's going on, again */
  INFO2(">>>>>>>> Got answer from %s:%d <<<<<<<<", 
	gras_socket_peer_name(from),gras_socket_peer_port(from));

  /* 10. Free the allocated resources, and shut GRAS down */
  gras_socket_close(toserver);
  gras_exit();
  INFO0("Done.");
  return 0;
} /* end_of_client */
