/* stoken - simple/static token ring                                        */

/* Copyright (c) 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras.h"

#define NBLOOPS 3
/*#define NBLOOPS 30000*/

XBT_LOG_NEW_DEFAULT_CATEGORY(SimpleToken,
                             "Messages specific to this example");

/* Function prototypes */
int node(int argc, char *argv[]);


/* **********************************************************************
 * Node code
 * **********************************************************************/

/* Global private data */
typedef struct {
  gras_socket_t sock;           /* server socket on which I hear */
  int remaining_loop;           /* number of loops to do until done */
  int create;                   /* whether I have to create the token */
  gras_socket_t tosuccessor;    /* how to connect to the successor on ring */
  double start_time;            /* to measure the elapsed time. Only used by the 
                                   node that creates the token */
} node_data_t;


/* Callback function */
static int node_cb_stoken_handler(gras_msg_cb_ctx_t ctx, void *payload)
{

  xbt_ex_t e;

  /* 1. Get the payload into the msg variable, and retrieve my caller */
  int msg = *(int *) payload;
  gras_socket_t expeditor = gras_msg_cb_ctx_from(ctx);


  /* 2. Retrieve the node's state (globals) */
  node_data_t *globals = (node_data_t *) gras_userdata_get();

  /* 3. Log which predecessor connected */
  int supersteps = 1;           /* only log every superstep loop */
  if (NBLOOPS >= 1000) {
    supersteps = 100;
  } else if (NBLOOPS >= 100) {
    supersteps = 10;
  } else if (NBLOOPS <= 10) {
    supersteps = 1;
  }
  if (globals->create && (!(globals->remaining_loop % supersteps))) {
    INFO1("Begin a new loop. Still to do: %d", globals->remaining_loop);
  } else if (!(globals->remaining_loop % supersteps)) {
    VERB3("Got token(%d) from %s remaining_loop=%d",
          msg, gras_socket_peer_name(expeditor), globals->remaining_loop);
  }

  /* 4. If the right shouldn't be stopped yet */
  if (globals->remaining_loop > 0) {
    msg += 1;

    DEBUG3("Send token(%d) to %s:%d", msg,
           gras_socket_peer_name(globals->tosuccessor),
           gras_socket_peer_port(globals->tosuccessor));

    /* 5. Send the token as payload of a stoken message to the successor */
    TRY {
      gras_msg_send(globals->tosuccessor, "stoken", &msg);

      /* 6. Deal with errors */
    }
    CATCH(e) {
      gras_socket_close(globals->sock);
      RETHROW0("Unable to forward token: %s");
    }

  }

  /* DO NOT CLOSE THE expeditor SOCKET since the client socket is
     reused by our predecessor.
     Closing this side would thus create troubles */

  /* 7. Decrease the remaining_loop integer. */
  globals->remaining_loop -= 1;

  /* 8. Repport the hop number to the user at the end */
  if (globals->remaining_loop == -1 && globals->create) {
    double elapsed = gras_os_time() - globals->start_time;
    INFO1("Shut down the token-ring. There was %d hops.", msg);
    VERB1("Elapsed time: %g", elapsed);
  }

  /* 9. Tell GRAS that we consummed this message */
  return 0;
}                               /* end_of_node_cb_stoken_handler */

int node(int argc, char *argv[])
{
  node_data_t *globals;

  const char *host;
  int myport;
  int peerport;

  xbt_ex_t e;

  /* 1. Init the GRAS infrastructure and declare my globals */
  gras_init(&argc, argv);
  globals = gras_userdata_new(node_data_t);


  /* 2. Get the successor's address. The command line overrides
     defaults when specified */
  host = "127.0.0.1";
  myport = 4000;
  peerport = 4000;
  if (argc >= 4) {
    myport = atoi(argv[1]);
    host = argv[2];
    peerport = atoi(argv[3]);
  }

  /* 3. Save successor's address in global var */
  globals->remaining_loop = NBLOOPS;
  globals->create = 0;
  globals->tosuccessor = NULL;

  if (!gras_os_getpid() % 100 || gras_if_RL())
    INFO3("Launch node listening on %d (successor on %s:%d)",
          myport, host, peerport);

  /* 4. Register the known messages.  */
  gras_msgtype_declare("stoken", gras_datadesc_by_name("int"));

  /* 5. Create my master socket for listening */
  globals->sock = gras_socket_server(myport);
  gras_os_sleep(1.0);           /* Make sure all server sockets are created */

  /* 6. Create socket to the successor on the ring */
  DEBUG2("Connect to my successor on %s:%d", host, peerport);

  globals->tosuccessor = gras_socket_client(host, peerport);

  /* 7. Register my callback */
  gras_cb_register("stoken", &node_cb_stoken_handler);

  /* 8. One node has to create the token at startup. 
     It's specified by a command line argument */
  if (argc >= 5
      && !strncmp("--create-token", argv[4], strlen("--create-token")))
    globals->create = 1;

  if (globals->create) {
    int token = 0;

    globals->start_time = gras_os_time();

    globals->remaining_loop = NBLOOPS - 1;

    INFO3("Create the token (with value %d) and send it to %s:%d",
          token, host, peerport);

    TRY {
      gras_msg_send(globals->tosuccessor, "stoken", &token);
    } CATCH(e) {
      RETHROW0("Unable to send the freshly created token: %s");
    }
  }

  /* 8. Wait up to 10 seconds for an incomming message to handle */
  while (globals->remaining_loop > (globals->create ? -1 : 0)) {
    gras_msg_handle(-1);

    DEBUG1("looping (remaining_loop=%d)", globals->remaining_loop);
  }

  gras_os_sleep(1.0);           /* FIXME: if the sender quited, receive fail */

  /* 9. Free the allocated resources, and shut GRAS down */
  gras_socket_close(globals->sock);
  gras_socket_close(globals->tosuccessor);
  free(globals);
  gras_exit();

  return 0;
}                               /* end_of_node */
