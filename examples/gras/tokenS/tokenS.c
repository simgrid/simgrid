/* $Id$ */

/* stoken - simple/static token ring                                        */

/* Copyright (c) 2005 Alexandre Colucci.                                    */
/* Copyright (c) 2005 Martin Quinson.                                       */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
 
#include "gras.h"
 
#define NBLOOPS 100

XBT_LOG_NEW_DEFAULT_CATEGORY(Token,"Messages specific to this example");

/* register messages which may be sent */
static void register_messages(void) {
  gras_msgtype_declare("stoken", gras_datadesc_by_name("int"));
}

/* Function prototypes */
int node (int argc,char *argv[]);


/* **********************************************************************
 * Node code
 * **********************************************************************/

/* Global private data */
typedef struct {
  gras_socket_t sock; /* server socket on which I hear */
  int remaining_loop; /* loop to do until done */
  int create;	      /* I have to create the token */
  gras_socket_t tosuccessor; /* how to connect to next peer on ring */
} node_data_t;


/* Callback function */
static int node_cb_stoken_handler(gras_socket_t  expeditor,
				  void          *payload_data) {
  
  xbt_ex_t e;
  
  /* 1. Get the payload into the msg variable */
  int msg=*(int*)payload_data;
 

  /* 2. Retrieve the node's state (globals) */
  node_data_t *globals=(node_data_t*)gras_userdata_get();
 
  /* 3. Log which predecessor connected */
  int supersteps = 1;
  if (NBLOOPS >= 1000) {
    supersteps = 100;
  } else if (NBLOOPS >= 100) {
    supersteps = 10;
  } 
  if (globals->create && (! (globals->remaining_loop % supersteps))) {
    INFO1("Begin a new loop. Still to do: %d", globals->remaining_loop);
  } else if (! (globals->remaining_loop % supersteps)) {
    VERB3("Got token(%d) from %s remaining_loop=%d", 
	  msg, gras_socket_peer_name(expeditor),globals->remaining_loop);
  }
  
  if (globals->remaining_loop > 0) {
     
    msg += 1;
     
    /* 5. I forward it to my successor */
    DEBUG3("Send token(%d) to %s:%d",
	   msg, 
	   gras_socket_peer_name(globals->tosuccessor),
	   gras_socket_peer_port(globals->tosuccessor));
     
     
    /* 6. Send it as payload of a stoken message to the successor */
    TRY {
      gras_msg_send(globals->tosuccessor, 
		    gras_msgtype_by_name("stoken"), &msg);
     
    /* 7. Deal with errors */
    } CATCH(e) {
      gras_socket_close(globals->sock);
      RETHROW0("Unable to forward token: %s");
    }
  
  }
	      
  /* DO NOT CLOSE THE expeditor SOCKET since the client socket is
     reused by our predecessor.
     Closing this side would thus create troubles */
  
  /* 9. Decrease the remaining_loop integer. */
  globals->remaining_loop -= 1;
   
  /* 10. Repport the hop number to the user at the end */
  if (globals->remaining_loop == -1 && globals->create) {
    INFO1("Shut down the token-ring. There was %d hops.",msg);
  }

  /* 11. Tell GRAS that we consummed this message */
  return 1;
} /* end_of_node_cb_stoken_handler */


int node (int argc,char *argv[]) {
  node_data_t *globals;
  
  const char *host;
  int   myport;
  int   peerport;
  
  /* 1. Init the GRAS infrastructure and declare my globals */
  gras_init(&argc,argv);
  globals=gras_userdata_new(node_data_t);
  
  
  /* 2. Get the successor's address. The command line overrides
        defaults when specified */
  host = "127.0.0.1";
  myport = 4000;
  peerport = 4000;
  if (argc >= 4) {
    myport=atoi(argv[1]);
    host=argv[2];
    peerport=atoi(argv[3]);
  }

  /* 3. Save successor's address in global var */
  globals->remaining_loop=NBLOOPS;
  globals->create = 0;
  globals->tosuccessor = NULL;
  	
  INFO4("Launch node %d (successor on %s:%d; listening on %d)",
	gras_os_getpid(), host,peerport, myport);

  /* 4. Create my master socket for listening */
  globals->sock = gras_socket_server(myport);
  gras_os_sleep(1.0); /* Make sure all server sockets are created */


  /* 5. Create socket to the successor on the ring */
  DEBUG2("Connect to my successor on %s:%d",host,peerport);

  globals->tosuccessor = gras_socket_client(host,peerport);
  
  /* 6. Register the known messages. This function is called twice here,
        but it's because this file also acts as regression test.
        No need to do so yourself of course. */
  register_messages();
  register_messages(); /* just to make sure it works ;) */
   
  /* 7. Register my callback */
  gras_cb_register(gras_msgtype_by_name("stoken"),&node_cb_stoken_handler);
  

  /* 8. One node has to create the token at startup. 
        It's specified by a command line argument */
  if (argc >= 5 && !strncmp("--create-token", argv[4],strlen(argv[4])))
    globals->create=1;

  if (globals->create) {
    int token = 0;
            
    globals->remaining_loop = NBLOOPS - 1;
      
    INFO3("Create the token (with value %d) and send it to %s:%d",
	  token, host, peerport);

    gras_msg_send(globals->tosuccessor,
		  gras_msgtype_by_name("stoken"), &token);
  } 
  
  /* 8. Wait up to 10 seconds for an incomming message to handle */
  while (globals->remaining_loop > (globals->create ? -1 : 0)) {
    gras_msg_handle(10.0);
  
    DEBUG1("looping (remaining_loop=%d)", globals->remaining_loop);
  }

  gras_os_sleep(1.0); /* FIXME: if the sender quited, receive fail */

  /* 9. Free the allocated resources, and shut GRAS down */ 
  gras_socket_close(globals->sock);
  gras_socket_close(globals->tosuccessor);
  free(globals);
  gras_exit();
  
  return 0;
} /* end_of_node */
