/* ALL2ALL - all2all of GRAS features                                       */

/* Copyright (c) 2006 Ahmed Harbaoui. All rights reserved.                  */

 /* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras.h"
#include "xbt/ex.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(all2all, "Messages specific to this example");

/* register data which may be sent (common to client and server) */
static void register_messages(void)
{
}

/* Function prototypes */
int receiver(int argc, char *argv[]);
int sender(int argc, char *argv[]);


/* **********************************************************************
 * Receiver code
 * **********************************************************************/
int receiver(int argc, char *argv[])
{

  int myport;                   /* port on which I receive stuff */
  int todo;                     /* amount of messages I should get */
  char *data;                   /* message content */

  gras_socket_t mysock;         /* socket on which other people contact me */
  gras_socket_t expeditor;      /* to notice who wrote me */

  /* Init the GRAS infrastructure and declare my globals */
  gras_init(&argc, argv);

  /* Get my settings from the command line */
  myport = atoi(argv[1]);
  todo = atoi(argv[2]);

  /* Register the known messages */
  gras_msgtype_declare("data", gras_datadesc_by_name("string"));

  /* Create my master socket */
  mysock = gras_socket_server(myport);

  /* Get the data */
  INFO2("Listening on port %d (expecting %d messages)",
        gras_socket_my_port(mysock), todo);
  while (todo > 0) {
    gras_msg_wait(60 /* wait up to one minute */ ,
                  "data", &expeditor, &data);
    todo--;
    free(data);

    INFO3("Got Data from %s:%d (still %d to go)",
          gras_socket_peer_name(expeditor), gras_socket_peer_port(expeditor),
          todo);

  }

  /* Free the allocated resources, and shut GRAS down */
  gras_socket_close(mysock);

  gras_exit();
  return 0;
}                               /* end_of_receiver */

/* **********************************************************************
 * Sender code
 * **********************************************************************/

int sender(int argc, char *argv[])
{

  unsigned int iter;            /* iterator */
  char *data;                   /* data exchanged */
  int datasize;                 /* size of message */
  xbt_peer_t h;                 /* iterator */
  int connected = 0;

  gras_socket_t peer = NULL;    /* socket to node */


  /* xbt_dynar for peers */
  xbt_dynar_t peers = xbt_dynar_new(sizeof(xbt_peer_t), &xbt_peer_free_voidp);

  /* Init the GRAS infrastructure and declare my globals */
  gras_init(&argc, argv);

  /* Get the node location from argc/argv */
  for (iter = 1; iter < argc - 1; iter++) {
    xbt_peer_t peer = xbt_peer_from_string(argv[iter]);
    xbt_dynar_push(peers, &peer);
  }

  datasize = atoi(argv[argc - 1]);

  data = (char *) malloc(datasize + 1); // allocation of datasize octets
  memset(data, 32, datasize);
  data[datasize] = '\0';

  INFO0("Launch current node");

  /* Register the known messages */
  gras_msgtype_declare("data", gras_datadesc_by_name("string"));


  /* write to the receivers */
  xbt_dynar_foreach(peers, iter, h) {
    connected = 0;
    while (!connected) {
      xbt_ex_t e;
      TRY {
        peer = gras_socket_client(h->name, h->port);
        connected = 1;
      }
      CATCH(e) {
        if (e.category != system_error  /*in RL */
            && e.category != mismatch_error /*in SG */ )
          RETHROW;
        xbt_ex_free(e);
        gras_os_sleep(0.01);
      }
    }
    gras_msg_send(peer, "data", &data);
    if (gras_if_SG()) {
      INFO2("  Sent Data from %s to %s", gras_os_myname(), h->name);
    } else {
      INFO0("  Sent Data");
    }

    gras_socket_close(peer);
  }

  /* Free the allocated resources, and shut GRAS down */
  free(data);
  xbt_dynar_free(&peers);

  gras_exit();
  return 0;
}                               /* end_of_sender */
