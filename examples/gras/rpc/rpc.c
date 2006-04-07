/* $Id$ */

/* rpc - demo of the RPC features in GRAS                                   */

/* Copyright (c) 2006 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(Rpc,"Messages specific to this example");

/* register messages which may be sent (common to client and server) */
static void register_messages(void) {
  gras_msgtype_declare_rpc("plain ping",
			   gras_datadesc_by_name("int"),
			   gras_datadesc_by_name("int"));

  gras_msgtype_declare_rpc("raise exception", NULL, NULL);
  gras_msgtype_declare_rpc("forward exception", NULL, NULL);
}

/* Function prototypes */
int server (int argc,char *argv[]);
int forwarder (int argc,char *argv[]);
int client (int argc,char *argv[]);

/* **********************************************************************
 * Client code
 * **********************************************************************/


int client(int argc,char *argv[]) {
  xbt_ex_t e; 
  gras_socket_t toserver=NULL; /* peer */
  gras_socket_t toforwarder=NULL; /* peer */

  

  int ping, pong, i;
  volatile int gotit=0;

  const char *host = "127.0.0.1";
        int   port = 4000;

  /* 1. Init the GRAS's infrastructure */
  gras_init(&argc, argv);
   
  /* 2. Get the server's address. The command line override defaults when specified */
  if (argc == 5) {
    host=argv[1];
    port=atoi(argv[2]);
  } 

  INFO2("Launch client (server on %s:%d)",host,port);
   
  /* 3. Wait for the server startup */
  gras_os_sleep(1);
   
  /* 4. Create a socket to speak to the server */
  TRY {
    toserver=gras_socket_client(host,port);
    toforwarder=gras_socket_client(argv[3],atoi(argv[4]));
  } CATCH(e) {
    RETHROW0("Unable to connect to the server: %s");
  }
  INFO2("Connected to %s:%d.",host,port);    


  /* 5. Register the messages. 
        See, it doesn't have to be done completely at the beginning,
	but only before use */
  register_messages();

  /* 6. Keep the user informed of what's going on */
  INFO2("Connected to server which is on %s:%d ", 
	gras_socket_peer_name(toserver),gras_socket_peer_port(toserver));

  /* 7. Prepare and send the ping message to the server */
  ping = 1234;
  TRY {
    gras_msg_rpccall(toserver, 6000.0,
		     gras_msgtype_by_name("plain ping"), &ping, &pong);
  } CATCH(e) {
    gras_socket_close(toserver);
    RETHROW0("Failed to execute a PING rpc on the server: %s");
  }

  /* 8. Keep the user informed of what's going on, again */
  INFO4("The answer to PING(%d) on %s:%d is PONG(%d)  ", 
	ping, 
	gras_socket_peer_name(toserver),gras_socket_peer_port(toserver),
	pong);

  /* 9. Call a RPC which raises an exception (to test that exception propagation works) */
  INFO0("Call the exception raising RPC");
  TRY {
    gras_msg_rpccall(toserver, 6000.0,
		     gras_msgtype_by_name("raise exception"), NULL, NULL);
  } CATCH(e) {
    gotit = 1;
  }
  if (!gotit) {
    THROW0(unknown_error,0,"Didn't got the remote exception!");
  }
  xbt_assert2(e.category == unknown_error, "Got wrong category: %d (instead of %d)", 
	      e.category,unknown_error);
  xbt_assert1(e.value == 42, "Got wrong value: %d (!=42)", e.value);
  xbt_assert1(!strcmp(e.msg,"Some error we will catch on client side"), 
	      "Got wrong message: %s", e.msg);;
  INFO0("Got the expected exception when calling the exception raising RPC");
  xbt_ex_free(&e);

  /* doxygen_ignore */
  for (i=0; i<5; i++) {
	
     INFO1("Call the exception raising RPC (i=%d)",i);
     TRY {
	gras_msg_rpccall(toserver, 6000.0,
			 gras_msgtype_by_name("raise exception"), NULL, NULL);
     } CATCH(e) {
	gotit = 1;
	xbt_ex_free(&e);
     }
     if (!gotit) {
	THROW0(unknown_error,0,"Didn't got the remote exception!");
     }
  }
  /* doxygen_resume */
  
  /* 9. Call a RPC which raises an exception (to test that exception propagation works) */
  for (i=0;i<2;i++) {
    INFO1("Call the exception raising RPC on the forwarder (i=%d)",i);
    TRY {
      gras_msg_rpccall(toforwarder, 6000.0,
		       gras_msgtype_by_name("forward exception"), NULL, NULL);
    } CATCH(e) {
      gotit = 1;
    }
    if (!gotit) {
      THROW0(unknown_error,0,"Didn't got the remote exception!");
    }
    xbt_assert1(e.category == unknown_error, "Got wrong category: %d", e.category);
    xbt_assert1(e.value == 42, "Got wrong value: %d (!=42)", e.value);
    xbt_assert1(!strcmp(e.msg,"Some error we will catch on client side"), 
		"Got wrong message: %s", e.msg);;
    INFO0("Got the expected exception when calling the exception raising RPC");
    xbt_ex_free(&e);
  }

  /* 11. Cleanup the place before leaving */
  gras_socket_close(toserver);
  gras_exit();
  INFO0("Done.");
  return 0;
} /* end_of_client */


/* **********************************************************************
 * Forwarder code
 * **********************************************************************/
typedef struct {
  gras_socket_t server;
} s_forward_data_t, *forward_data_t;

static int server_cb_forward_ex(gras_msg_cb_ctx_t ctx,
			      void             *payload_data) {
  forward_data_t fdata=gras_userdata_get();

  gras_msg_rpccall(fdata->server, 60, gras_msgtype_by_name("raise exception"),NULL,NULL);
  return 1;
}

int forwarder (int argc,char *argv[]) {
  gras_socket_t mysock;
  int port;
  forward_data_t fdata;

  gras_init(&argc,argv);

  xbt_assert(argc == 4);

  fdata=gras_userdata_new(s_forward_data_t);
  port=atoi(argv[1]);

  INFO1("Launch forwarder (port=%d)", port);
  mysock = gras_socket_server(port);

  gras_os_sleep(0.1); /* wait for the server to be ready */
  fdata->server=gras_socket_client(argv[2],atoi(argv[3]));

  register_messages();
  gras_cb_register(gras_msgtype_by_name("forward exception"),&server_cb_forward_ex);

  gras_msg_handle(600.0); /* deal with the first forwarded request */
  gras_msg_handle(600.0); /* deal with the second forwarded request */
  gras_socket_close(mysock);
  gras_socket_close(fdata->server);
  free(fdata);
  gras_exit();
  INFO0("Done.");
  return 0;
}

/* **********************************************************************
 * Server code
 * **********************************************************************/

static int server_cb_raise_ex(gras_msg_cb_ctx_t ctx,
				void             *payload_data) {
  THROW0(unknown_error,42,"Some error we will catch on client side");
}

static int server_cb_ping(gras_msg_cb_ctx_t ctx,
			  void             *payload_data) {
			     
  /* 1. Get the payload into the msg variable, and retrieve who called us */
  int msg=*(int*)payload_data;
  gras_socket_t expeditor = gras_msg_cb_ctx_from(ctx);
   
  /* 2. Log which client connected */
  INFO3("Got message PING(%d) from %s:%d ", 
	msg, 
 	gras_socket_peer_name(expeditor), gras_socket_peer_port(expeditor));
  
  /* 4. Change the value of the msg variable */
  msg = 4321;

  /* 5. Return as result */
  gras_msg_rpcreturn(6000,ctx,&msg);
  INFO0("Answered with PONG(4321)");
     
  /* 6. Cleanups, if any */

  /* 7. Tell GRAS that we consummed this message */
  return 1;
} /* end_of_server_cb_ping */


int server (int argc,char *argv[]) {
  gras_socket_t mysock;

  int port = 4000;
  int i;
  
  /* 1. Init the GRAS infrastructure */
  gras_init(&argc,argv);
   
  /* 2. Get the port I should listen on from the command line, if specified */
  if (argc == 2) 
    port=atoi(argv[1]);

  INFO1("Launch server (port=%d)", port);

  /* 3. Create my master socket */
  mysock = gras_socket_server(port);

  /* 4. Register the known messages and register my callback */
  register_messages();
  gras_cb_register(gras_msgtype_by_name("plain ping"),&server_cb_ping);
  gras_cb_register(gras_msgtype_by_name("raise exception"),&server_cb_raise_ex);

  INFO1("Listening on port %d ", gras_socket_my_port(mysock));

  /* 5. Wait for the ping incomming messages */
  gras_msg_handle(600.0);
  /* 6. Wait for the exception raiser incomming messages */
  gras_msg_handle(600.0);
  /* doxygen_ignore */
  for (i=0; i<5; i++)
     gras_msg_handle(600.0);
  /* doxygen_resume */

  /* 7. Wait for the exception forwarder incomming messages (twice, to see if it triggers bugs) */
  gras_msg_handle(600.0); 
  gras_msg_handle(600.0); 
   
  /* 8. Free the allocated resources, and shut GRAS down */
  gras_socket_close(mysock);
  gras_exit();
   
  INFO0("Done.");
  return 0;
} /* end_of_server */

