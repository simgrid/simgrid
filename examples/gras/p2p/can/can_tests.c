/* Broken Peer-To-Peer CAN simulator                                        */

/* Copyright (c) 2006, 2007, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <time.h>
//#include "gras.h"
#include "types.h"              // header containing the typedef struct of a node

XBT_LOG_NEW_DEFAULT_CATEGORY(can, "Messages specific to this example");

// struct of a "nuke" message, when a node send a nuke to (xId;yId).
GRAS_DEFINE_TYPE(s_nuke, struct s_nuke {
                 int xId; int yId; char host[1024];     // original expeditor..
                 int port;      // ..and his port.
                 int version;   // fun.
                 };);

typedef struct s_nuke nuke_t;

// the function that start the **** War of the Nodes ****
int start_war(int argc, char **argv);
int start_war(int argc, char **argv)
{
  gras_socket_t temp_sock = NULL;
  nuke_t nuke_msg;
  xbt_ex_t e;                   // the error variable used in TRY.. CATCH tokens.
  //return 0; // in order to inhibit the War of the Nodes 
  gras_init(&argc, argv);
  gras_os_sleep((15 - gras_os_getpid()) * 20 + 200);    // wait a bit.


  TRY {                         // contacting the bad guy that will launch the War.
    temp_sock = gras_socket_client(gras_os_myname(), atoi(argv[1]));
  } CATCH(e) {
    RETHROW0("Unable to connect known host so as to declare WAR!: %s");
  }


  nuke_msg.xId = -1;
  nuke_msg.yId = -1;
  nuke_msg.version = atoi(argv[2]);
  strcpy(nuke_msg.host, gras_os_myname());
  nuke_msg.port = atoi(argv[1]);

  TRY {
    gras_msg_send(temp_sock, "can_nuke", &nuke_msg);
  } CATCH(e) {
    gras_socket_close(temp_sock);
    RETHROW0
        ("Unable to contact known host so as to declare WAR!!!!!!!!!!!!!!!!!!!!!: %s");
  }
  gras_socket_close(temp_sock); // spare.
  gras_exit();                  // spare.
  return 0;
}

// the function thaht send the nuke "msg" on (xId;yId), if it's not on me :p.
static int send_nuke(nuke_t * msg, int xId, int yId)
{
  node_data_t *globals = (node_data_t *) gras_userdata_get();
  gras_socket_t temp_sock = NULL;
  xbt_ex_t e;                   // the error variable used in TRY.. CATCH tokens.

  if (xId >= globals->x1 && xId <= globals->x2 && yId >= globals->y1
      && yId <= globals->y2) {
    INFO0("Nuclear launch missed");
    return 0;
  } else {
    char host[1024];
    int port = 0;

    if (xId < globals->x1) {
      strcpy(host, globals->west_host);
      port = globals->west_port;
    } else if (xId > globals->x2) {
      strcpy(host, globals->east_host);
      port = globals->east_port;
    } else if (yId < globals->y1) {
      strcpy(host, globals->south_host);
      port = globals->south_port;
    } else if (yId > globals->y2) {
      strcpy(host, globals->north_host);
      port = globals->north_port;
    }

    msg->xId = xId;
    msg->yId = yId;



    TRY {                       // sending the nuke.
      temp_sock = gras_socket_client(host, port);
    }
    CATCH(e) {
      RETHROW0("Unable to connect the nuke!: %s");
    }
    //INFO4("%s ON %s %d %d <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<",globals->host,host,xId,yId);
    TRY {
      gras_msg_send(temp_sock, "can_nuke", msg);
    }
    CATCH(e) {
      RETHROW0("Unable to send the nuke!: %s");
    }
    gras_socket_close(temp_sock);
    INFO4("Nuke launched by %s to %s for (%d;%d)", globals->host, host,
          msg->xId, msg->yId);
    return 1;
  }
}


static int node_nuke_handler(gras_msg_cb_ctx_t ctx, void *payload_data)
{
  gras_socket_t expeditor = gras_msg_cb_ctx_from(ctx);
  nuke_t *incoming = (nuke_t *) payload_data;
  node_data_t *globals = (node_data_t *) gras_userdata_get();

  int x;
  int y;
  nuke_t nuke_msg;              // writing my name one the nuke.
  gras_socket_t temp_sock = NULL;
  xbt_ex_t e;                   // the error variable used in TRY.. CATCH tokens.


  if (incoming->xId == -1) {    // i must start the War
    INFO2("%s:%d declare the WAR!!!!!!!!!!!!!!!!!", globals->host,
          globals->port);
    srand((unsigned int) time((time_t *) NULL));

    do {
      x = (int) (1000.0 * rand() / (RAND_MAX + 1.0));
      y = (int) (1000.0 * rand() / (RAND_MAX + 1.0));
    }
    while (send_nuke(incoming, x, y) == 0);

  } else if (incoming->xId >= globals->x1 && incoming->xId <= globals->x2 && incoming->yId >= globals->y1 && incoming->yId <= globals->y2) {    // the nuke crash on my area..
    if (globals->version == incoming->version)  // ..but i'm dead.
      INFO0("I'm already dead :p");
    else if ((incoming->xId - globals->xId) / 60 == 0 && (incoming->yId - globals->yId) / 60 == 0) {    // ..and it's on me, so i die :X.
      globals->version = incoming->version;
      INFO2("Euuuaarrrgghhhh...   %s killed %s !!!!!!!!!!!!!!!!!",
            incoming->host, globals->host);
    } else {                    // and it miss me, i angry and i send my own nuke!
      INFO1("%s was missed, and counteract!", globals->host);
      /*int x1=(int)(1000.0*rand()/(RAND_MAX+1.0));
         int y1=(int)(1000.0*rand()/(RAND_MAX+1.0));
         int x2=(int)(1000.0*rand()/(RAND_MAX+1.0));
         int y2=(int)(1000.0*rand()/(RAND_MAX+1.0));
         int x3=(int)(1000.0*rand()/(RAND_MAX+1.0));
         int y3=(int)(1000.0*rand()/(RAND_MAX+1.0));
         int x4=(int)(1000.0*rand()/(RAND_MAX+1.0));
         int y4=(int)(1000.0*rand()/(RAND_MAX+1.0)); */


      nuke_msg.version = incoming->version;
      strcpy(nuke_msg.host, globals->host);
      nuke_msg.port = globals->port;


      do {
        x = (int) (1000.0 * rand() / (RAND_MAX + 1.0));
        y = (int) (1000.0 * rand() / (RAND_MAX + 1.0));
      }
      while (send_nuke(&nuke_msg, x, y) == 0);  // and sending if it's not on me.
    }
  } else {                      // the nuke isn't for me, so i forward her.
    char host[1024];
    int port = 0;

    if (incoming->xId < globals->x1) {
      strcpy(host, globals->west_host);
      port = globals->west_port;
    } else if (incoming->xId > globals->x2) {
      strcpy(host, globals->east_host);
      port = globals->east_port;
    } else if (incoming->yId < globals->y1) {
      strcpy(host, globals->south_host);
      port = globals->south_port;
    } else if (incoming->yId > globals->y2) {
      strcpy(host, globals->north_host);
      port = globals->north_port;
    }



    TRY {
      temp_sock = gras_socket_client(host, port);
    }
    CATCH(e) {
      RETHROW0("Unable to connect the nuke!: %s");
    }
    TRY {
      gras_msg_send(temp_sock, "can_nuke", incoming);
    }
    CATCH(e) {
      RETHROW0("Unable to send the nuke!: %s");
    }
    INFO4("Nuke re-aimed by %s to %s for (%d;%d)", globals->host, host,
          incoming->xId, incoming->yId);
    gras_socket_close(temp_sock);
  }
  gras_socket_close(expeditor); // spare.

  TRY {
    gras_msg_handle(10000.0);   // wait a bit, in case of..
  }
  CATCH(e) {
    INFO4("My area is [%d;%d;%d;%d]", globals->x1, globals->x2,
          globals->y1, globals->y2);
    //INFO0("Closing node, all has been done!");
  }
  return 0;
}

// END
