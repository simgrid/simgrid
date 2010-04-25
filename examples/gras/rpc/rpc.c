/* rpc - demo of the RPC features in GRAS                                   */

/* Copyright (c) 2006 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(Rpc, "Messages specific to this example");

/* register messages which may be sent (common to client and server) */
static void register_messages(void)
{
  gras_msgtype_declare_rpc("plain ping",
                           gras_datadesc_by_name("int"),
                           gras_datadesc_by_name("int"));

  gras_msgtype_declare_rpc("raise exception", NULL, NULL);
  gras_msgtype_declare_rpc("forward exception", NULL, NULL);
  gras_msgtype_declare("kill", NULL);
}

/* Function prototypes */
int server(int argc, char *argv[]);
int forwarder(int argc, char *argv[]);
int client(int argc, char *argv[]);

#define exception_msg       "Error for the client"
#define exception_raising() THROW0(unknown_error,42,exception_msg)

static void exception_catching(void)
{
  int gotit = 0, i;
  xbt_ex_t e;

  for (i = 0; i < 5; i++) {
    gotit = 0;
    TRY {
      exception_raising();
    }
    CATCH(e) {
      gotit = 1;
    }
    if (!gotit) {
      THROW0(unknown_error, 0, "Didn't got the remote exception!");
    }
    xbt_assert2(e.category == unknown_error,
                "Got wrong category: %d (instead of %d)", e.category,
                unknown_error);
    xbt_assert1(e.value == 42, "Got wrong value: %d (!=42)", e.value);
    xbt_assert1(!strncmp(e.msg, exception_msg, strlen(exception_msg)),
                "Got wrong message: %s", e.msg);
    xbt_ex_free(e);
  }
}

/* **********************************************************************
 * Client code
 * **********************************************************************/


int client(int argc, char *argv[])
{
  xbt_ex_t e;
  gras_socket_t toserver = NULL;        /* peer */
  gras_socket_t toforwarder = NULL;     /* peer */

  int ping, pong, i;
  volatile int gotit = 0;


  const char *host = "127.0.0.1";
  int port = 4000;

  memset(&e, 0, sizeof(xbt_ex_t));

  /* 1. Init the GRAS's infrastructure */
  gras_init(&argc, argv);

  /* 2. Get the server's address. The command line override defaults when specified */
  if (argc == 5) {
    host = argv[1];
    port = atoi(argv[2]);
  }
  INFO2("Launch client (server on %s:%d)", host, port);

  exception_catching();

  /* 3. Wait for the server & forwarder startup */
  gras_os_sleep(2);

  /* 4. Create a socket to speak to the server */
  TRY {
    exception_catching();
    toserver = gras_socket_client(host, port);
    toforwarder = gras_socket_client(argv[3], atoi(argv[4]));
  }
  CATCH(e) {
    RETHROW0("Unable to connect to the server: %s");
  }
  INFO2("Connected to %s:%d.", host, port);


  /* 5. Register the messages.
     See, it doesn't have to be done completely at the beginning,
     but only before use */
  exception_catching();
  register_messages();

  /* 6. Keep the user informed of what's going on */
  INFO2("Connected to server which is on %s:%d",
        gras_socket_peer_name(toserver), gras_socket_peer_port(toserver));

  /* 7. Prepare and send the ping message to the server */
  ping = 1234;
  TRY {
    exception_catching();
    gras_msg_rpccall(toserver, 6000.0, "plain ping", &ping, &pong);
  }
  CATCH(e) {
    gras_socket_close(toserver);
    RETHROW0("Failed to execute a PING rpc on the server: %s");
  }
  exception_catching();

  /* 8. Keep the user informed of what's going on, again */
  INFO4("The answer to PING(%d) on %s:%d is PONG(%d)",
        ping,
        gras_socket_peer_name(toserver), gras_socket_peer_port(toserver),
        pong);

  /* 9. Call a RPC which raises an exception (to test exception propagation) */
  INFO0("Call the exception raising RPC");
  TRY {
    gras_msg_rpccall(toserver, 6000.0, "raise exception", NULL, NULL);
  }
  CATCH(e) {
    gotit = 1;
    xbt_assert2(e.category == unknown_error,
                "Got wrong category: %d (instead of %d)",
                e.category, unknown_error);
    xbt_assert1(e.value == 42, "Got wrong value: %d (!=42)", e.value);
    xbt_assert1(!strncmp(e.msg, exception_msg, strlen(exception_msg)),
                "Got wrong message: %s", e.msg);
    INFO0
      ("Got the expected exception when calling the exception raising RPC");
    xbt_ex_free(e);
  }

  if (!gotit)
    THROW0(unknown_error, 0, "Didn't got the remote exception!");

  INFO0("Called the exception raising RPC");
  exception_catching();

  /* doxygen_ignore */
  for (i = 0; i < 5; i++) {

    INFO1("Call the exception raising RPC (i=%d)", i);
    TRY {
      gras_msg_rpccall(toserver, 6000.0, "raise exception", NULL, NULL);
    }
    CATCH(e) {
      gotit = 1;
      xbt_ex_free(e);
    }
    if (!gotit) {
      THROW0(unknown_error, 0, "Didn't got the remote exception!");
    }
  }
  /* doxygen_resume */

  /* 9. Call a RPC which raises an exception (to test that exception propagation works) */
  for (i = 0; i < 5; i++) {
    INFO1("Call the exception raising RPC on the forwarder (i=%d)", i);
    TRY {
      gras_msg_rpccall(toforwarder, 6000.0, "forward exception", NULL, NULL);
    }
    CATCH(e) {
      gotit = 1;
    }
    if (!gotit) {
      THROW0(unknown_error, 0, "Didn't got the remote exception!");
    }
    xbt_assert1(e.value == 42, "Got wrong value: %d (!=42)", e.value);
    xbt_assert1(!strncmp(e.msg, exception_msg, strlen(exception_msg)),
                "Got wrong message: %s", e.msg);
    xbt_assert2(e.category == unknown_error,
                "Got wrong category: %d (instead of %d)",
                e.category, unknown_error);
    INFO0
      ("Got the expected exception when calling the exception raising RPC");
    xbt_ex_free(e);
    exception_catching();
  }

  INFO2("Ask %s:%d to die", gras_socket_peer_name(toforwarder),
        gras_socket_peer_port(toforwarder));
  gras_msg_send(toforwarder, "kill", NULL);
  INFO2("Ask %s:%d to die", gras_socket_peer_name(toserver),
        gras_socket_peer_port(toserver));
  gras_msg_send(toserver, "kill", NULL);

  /* 11. Cleanup the place before leaving */
  gras_socket_close(toserver);
  gras_socket_close(toforwarder);
  INFO0("Done.");
  gras_exit();
  return 0;
}                               /* end_of_client */


/* **********************************************************************
 * Forwarder code
 * **********************************************************************/
typedef struct {
  gras_socket_t server;
  int done;
} s_forward_data_t, *forward_data_t;

static int forwarder_cb_kill(gras_msg_cb_ctx_t ctx, void *payload_data)
{
  forward_data_t fdata;
  gras_socket_t expeditor = gras_msg_cb_ctx_from(ctx);
  INFO2("Asked to die by %s:%d", gras_socket_peer_name(expeditor),
        gras_socket_peer_port(expeditor));
  fdata = gras_userdata_get();
  fdata->done = 1;
  return 0;
}

static int forwarder_cb_forward_ex(gras_msg_cb_ctx_t ctx, void *payload_data)
{
  forward_data_t fdata = gras_userdata_get();

  INFO0("Forward a request");
  gras_msg_rpccall(fdata->server, 60, "raise exception", NULL, NULL);
  return 0;
}

int forwarder(int argc, char *argv[])
{
  gras_socket_t mysock;
  int port;
  forward_data_t fdata;

  gras_init(&argc, argv);

  xbt_assert(argc == 4);

  fdata = gras_userdata_new(s_forward_data_t);
  fdata->done = 0;
  port = atoi(argv[1]);

  INFO1("Launch forwarder (port=%d)", port);
  mysock = gras_socket_server(port);

  gras_os_sleep(1);             /* wait for the server to be ready */
  fdata->server = gras_socket_client(argv[2], atoi(argv[3]));

  register_messages();
  gras_cb_register("forward exception", &forwarder_cb_forward_ex);
  gras_cb_register("kill", &forwarder_cb_kill);

  while (!fdata->done) {
    gras_msg_handle(600.0);
  }

  gras_socket_close(mysock);
  gras_socket_close(fdata->server);
  free(fdata);
  INFO0("Done.");
  gras_exit();
  return 0;
}

/* **********************************************************************
 * Server code
 * **********************************************************************/
typedef struct {
  gras_socket_t server;
  int done;
} s_server_data_t, *server_data_t;

static int server_cb_kill(gras_msg_cb_ctx_t ctx, void *payload_data)
{
  gras_socket_t expeditor = gras_msg_cb_ctx_from(ctx);
  server_data_t sdata;

  INFO2("Asked to die by %s:%d", gras_socket_peer_name(expeditor),
        gras_socket_peer_port(expeditor));

  sdata = gras_userdata_get();
  sdata->done = 1;
  return 0;
}

static int server_cb_raise_ex(gras_msg_cb_ctx_t ctx, void *payload_data)
{
  exception_raising();
  return 0;
}

static int server_cb_ping(gras_msg_cb_ctx_t ctx, void *payload_data)
{

  /* 1. Get the payload into the msg variable, and retrieve who called us */
  int msg = *(int *) payload_data;
  gras_socket_t expeditor = gras_msg_cb_ctx_from(ctx);

  /* 2. Log which client connected */
  INFO3("Got message PING(%d) from %s:%d",
        msg,
        gras_socket_peer_name(expeditor), gras_socket_peer_port(expeditor));

  /* 4. Change the value of the msg variable */
  msg = 4321;

  /* 5. Return as result */
  gras_msg_rpcreturn(6000, ctx, &msg);
  INFO0("Answered with PONG(4321)");

  /* 6. Cleanups, if any */

  /* 7. Tell GRAS that we consummed this message */
  return 0;
}                               /* end_of_server_cb_ping */


int server(int argc, char *argv[])
{
  gras_socket_t mysock;
  server_data_t sdata;

  int port = 4000;

  /* 1. Init the GRAS infrastructure */
  gras_init(&argc, argv);

  /* 2. Get the port I should listen on from the command line, if specified */
  if (argc == 2)
    port = atoi(argv[1]);

  sdata = gras_userdata_new(s_server_data_t);
  sdata->done = 0;

  INFO1("Launch server (port=%d)", port);

  /* 3. Create my master socket */
  mysock = gras_socket_server(port);

  /* 4. Register the known messages and register my callback */
  register_messages();
  gras_cb_register("plain ping", &server_cb_ping);
  gras_cb_register("raise exception", &server_cb_raise_ex);
  gras_cb_register("kill", &server_cb_kill);

  INFO1("Listening on port %d", gras_socket_my_port(mysock));

  /* 5. Wait for the ping incomming messages */

  /** \bug if the server is gone before the forwarder tries to connect,
     it dies awfully with the following message. The problem stands somewhere
     at the interface between the gras_socket_t and the msg mess. There is thus
     no way for me to dive into this before this interface is rewritten
==15875== Invalid read of size 4
==15875==    at 0x408B805: find_port (transport_plugin_sg.c:68)
==15875==    by 0x408BD64: gras_trp_sg_socket_client (transport_plugin_sg.c:115)
==15875==    by 0x404A38B: gras_socket_client_ext (transport.c:255)
==15875==    by 0x404A605: gras_socket_client (transport.c:288)
==15875==    by 0x804B49D: forwarder (rpc.c:245)
==15875==    by 0x80491FB: launch_forwarder (_rpc_simulator.c:52)
==15875==    by 0x406780B: __context_wrapper (context.c:164)
==15875==    by 0x41A6CB3: pthread_start_thread (manager.c:310)
==15875==    by 0x42AA549: clone (clone.S:119)
==15875==  Address 0x433B49C is 44 bytes inside a block of size 48 free'd
==15875==    at 0x401CF46: free (vg_replace_malloc.c:235)
==15875==    by 0x408F1FA: gras_process_exit (sg_process.c:117)
==15875==    by 0x4049386: gras_exit (gras.c:64)
==15875==    by 0x804B936: server (rpc.c:345)
==15875==    by 0x80492B1: launch_server (_rpc_simulator.c:69)
==15875==    by 0x406780B: __context_wrapper (context.c:164)
==15875==    by 0x41A6CB3: pthread_start_thread (manager.c:310)
==15875==    by 0x42AA549: clone (clone.S:119)
  */
  while (!sdata->done) {
    gras_msg_handle(600.0);
    exception_catching();
  }

  /* 8. Free the allocated resources, and shut GRAS down */
  free(sdata);
  gras_socket_close(mysock);
  INFO0("Done.");
  gras_exit();

  return 0;
}                               /* end_of_server */
