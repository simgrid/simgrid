/* rpc - demo of the RPC features in GRAS                                   */

/* Copyright (c) 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(Rpc, "Messages specific to this example");

/* register messages which may be sent (common to client and server) */
static void register_messages(void)
{
  gras_msgtype_declare_rpc("plain ping",
                           xbt_datadesc_by_name("int"),
                           xbt_datadesc_by_name("int"));

  gras_msgtype_declare_rpc("raise exception", NULL, NULL);
  gras_msgtype_declare_rpc("forward exception", NULL, NULL);
  gras_msgtype_declare("kill", NULL);
}

/* Function prototypes */
int server(int argc, char *argv[]);
int forwarder(int argc, char *argv[]);
int client(int argc, char *argv[]);

#define exception_msg       "Error for the client"
#define exception_raising() THROWF(unknown_error,42,exception_msg)

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
      xbt_assert(e.category == unknown_error,
                 "Got wrong category: %d (instead of %d)", e.category,
                 unknown_error);
      xbt_assert(e.value == 42, "Got wrong value: %d (!=42)", e.value);
      xbt_assert(!strncmp(e.msg, exception_msg, strlen(exception_msg)),
                 "Got wrong message: %s", e.msg);
      xbt_ex_free(e);
    }
    if (!gotit) {
      THROWF(unknown_error, 0, "Didn't got the remote exception!");
    }
  }
}

/* **********************************************************************
 * Client code
 * **********************************************************************/

static void client_create_sockets(xbt_socket_t *toserver,
                                  xbt_socket_t *toforwarder,
                                  const char *srv_host, int srv_port,
                                  const char *fwd_host, int fwd_port)
{
  TRY {
    exception_catching();
    *toserver = gras_socket_client(srv_host, srv_port);
    *toforwarder = gras_socket_client(fwd_host, fwd_port);
  }
  CATCH_ANONYMOUS {
    RETHROWF("Unable to connect to the server: %s");
  }
}

int client(int argc, char *argv[])
{
  xbt_ex_t e;
  xbt_socket_t toserver = NULL;        /* peer */
  xbt_socket_t toforwarder = NULL;     /* peer */

  int ping, pong, i;
  volatile int gotit = 0;


  const char *host = "127.0.0.1";
  int port = 4001;

  memset(&e, 0, sizeof(xbt_ex_t));

  /* 1. Init the GRAS's infrastructure */
  gras_init(&argc, argv);

  /* 2. Get the server's address. The command line override defaults when specified */
  if (argc == 5) {
    host = argv[1];
    port = atoi(argv[2]);
  }
  XBT_INFO("Launch client (server on %s:%d)", host, port);

  exception_catching();

  /* 3. Wait for the server & forwarder startup */
  gras_os_sleep(2);

  /* 4. Create a socket to speak to the server */
  client_create_sockets(&toserver, &toforwarder,
                        host, port, argv[3], atoi(argv[4]));
  XBT_INFO("Connected to %s:%d.", host, port);


  /* 5. Register the messages.
     See, it doesn't have to be done completely at the beginning,
     but only before use */
  exception_catching();
  register_messages();

  /* 6. Keep the user informed of what's going on */
  XBT_INFO("Connected to server which is on %s:%d",
        xbt_socket_peer_name(toserver), xbt_socket_peer_port(toserver));

  /* 7. Prepare and send the ping message to the server */
  ping = 1234;
  TRY {
    exception_catching();
    gras_msg_rpccall(toserver, 6000.0, "plain ping", &ping, &pong);
  }
  CATCH_ANONYMOUS {
    gras_socket_close(toserver);
    RETHROWF("Failed to execute a PING rpc on the server: %s");
  }
  exception_catching();

  /* 8. Keep the user informed of what's going on, again */
  XBT_INFO("The answer to PING(%d) on %s:%d is PONG(%d)",
        ping,
        xbt_socket_peer_name(toserver), xbt_socket_peer_port(toserver),
        pong);

  /* 9. Call a RPC which raises an exception (to test exception propagation) */
  XBT_INFO("Call the exception raising RPC");
  TRY {
    gras_msg_rpccall(toserver, 6000.0, "raise exception", NULL, NULL);
  }
  CATCH(e) {
    gotit = 1;
    xbt_assert(e.category == unknown_error,
                "Got wrong category: %d (instead of %d)",
                e.category, unknown_error);
    xbt_assert(e.value == 42, "Got wrong value: %d (!=42)", e.value);
    xbt_assert(!strncmp(e.msg, exception_msg, strlen(exception_msg)),
                "Got wrong message: %s", e.msg);
    XBT_INFO
        ("Got the expected exception when calling the exception raising RPC");
    xbt_ex_free(e);
  }

  if (!gotit)
    THROWF(unknown_error, 0, "Didn't got the remote exception!");

  XBT_INFO("Called the exception raising RPC");
  exception_catching();

  /* doxygen_ignore */
  for (i = 0; i < 5; i++) {

    XBT_INFO("Call the exception raising RPC (i=%d)", i);
    TRY {
      gras_msg_rpccall(toserver, 6000.0, "raise exception", NULL, NULL);
    }
    CATCH(e) {
      gotit = 1;
      xbt_ex_free(e);
    }
    if (!gotit) {
      THROWF(unknown_error, 0, "Didn't got the remote exception!");
    }
  }
  /* doxygen_resume */

  /* 9. Call a RPC which raises an exception (to test that exception propagation works) */
  for (i = 0; i < 5; i++) {
    XBT_INFO("Call the exception raising RPC on the forwarder (i=%d)", i);
    TRY {
      gras_msg_rpccall(toforwarder, 6000.0, "forward exception", NULL,
                       NULL);
    }
    CATCH(e) {
      gotit = 1;
      xbt_assert(e.value == 42, "Got wrong value: %d (!=42)", e.value);
      xbt_assert(!strncmp(e.msg, exception_msg, strlen(exception_msg)),
                 "Got wrong message: %s", e.msg);
      xbt_assert(e.category == unknown_error,
                 "Got wrong category: %d (instead of %d)",
                 e.category, unknown_error);
      XBT_INFO
        ("Got the expected exception when calling the exception raising RPC");
      xbt_ex_free(e);
    }
    if (!gotit) {
      THROWF(unknown_error, 0, "Didn't got the remote exception!");
    }
    exception_catching();
  }

  XBT_INFO("Ask %s:%d to die", xbt_socket_peer_name(toforwarder),
        xbt_socket_peer_port(toforwarder));
  gras_msg_send(toforwarder, "kill", NULL);
  XBT_INFO("Ask %s:%d to die", xbt_socket_peer_name(toserver),
        xbt_socket_peer_port(toserver));
  gras_msg_send(toserver, "kill", NULL);

  /* 11. Cleanup the place before leaving */
  gras_socket_close(toserver);
  gras_socket_close(toforwarder);
  XBT_INFO("Done.");
  gras_exit();
  return 0;
}                               /* end_of_client */


/* **********************************************************************
 * Forwarder code
 * **********************************************************************/
typedef struct {
  xbt_socket_t server;
  int done;
} s_forward_data_t, *forward_data_t;

static int forwarder_cb_kill(gras_msg_cb_ctx_t ctx, void *payload_data)
{
  forward_data_t fdata;
  xbt_socket_t expeditor = gras_msg_cb_ctx_from(ctx);
  XBT_INFO("Asked to die by %s:%d", xbt_socket_peer_name(expeditor),
        xbt_socket_peer_port(expeditor));
  fdata = gras_userdata_get();
  fdata->done = 1;
  return 0;
}

static int forwarder_cb_forward_ex(gras_msg_cb_ctx_t ctx,
                                   void *payload_data)
{
  forward_data_t fdata = gras_userdata_get();

  XBT_INFO("Forward a request");
  gras_msg_rpccall(fdata->server, 60, "raise exception", NULL, NULL);
  return 0;
}

int forwarder(int argc, char *argv[])
{
  xbt_socket_t mysock;
  int port;
  forward_data_t fdata;

  gras_init(&argc, argv);

  xbt_assert(argc == 4);

  fdata = gras_userdata_new(s_forward_data_t);
  fdata->done = 0;
  port = atoi(argv[1]);

  XBT_INFO("Launch forwarder (port=%d)", port);
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
  XBT_INFO("Done.");
  gras_exit();
  return 0;
}

/* **********************************************************************
 * Server code
 * **********************************************************************/
typedef struct {
  xbt_socket_t server;
  int done;
} s_server_data_t, *server_data_t;

static int server_cb_kill(gras_msg_cb_ctx_t ctx, void *payload_data)
{
  xbt_socket_t expeditor = gras_msg_cb_ctx_from(ctx);
  server_data_t sdata;

  XBT_INFO("Asked to die by %s:%d", xbt_socket_peer_name(expeditor),
        xbt_socket_peer_port(expeditor));

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
  xbt_socket_t expeditor = gras_msg_cb_ctx_from(ctx);

  /* 2. Log which client connected */
  XBT_INFO("Got message PING(%d) from %s:%d",
        msg,
        xbt_socket_peer_name(expeditor),
        xbt_socket_peer_port(expeditor));

  /* 4. Change the value of the msg variable */
  msg = 4321;

  /* 5. Return as result */
  gras_msg_rpcreturn(6000, ctx, &msg);
  XBT_INFO("Answered with PONG(4321)");

  /* 6. Cleanups, if any */

  /* 7. Tell GRAS that we consummed this message */
  return 0;
}                               /* end_of_server_cb_ping */


int server(int argc, char *argv[])
{
  xbt_socket_t mysock;
  server_data_t sdata;

  int port = 4001;

  /* 1. Init the GRAS infrastructure */
  gras_init(&argc, argv);

  /* 2. Get the port I should listen on from the command line, if specified */
  if (argc == 2)
    port = atoi(argv[1]);

  sdata = gras_userdata_new(s_server_data_t);
  sdata->done = 0;

  XBT_INFO("Launch server (port=%d)", port);

  /* 3. Create my master socket */
  mysock = gras_socket_server(port);

  /* 4. Register the known messages and register my callback */
  register_messages();
  gras_cb_register("plain ping", &server_cb_ping);
  gras_cb_register("raise exception", &server_cb_raise_ex);
  gras_cb_register("kill", &server_cb_kill);

  XBT_INFO("Listening on port %d", xbt_socket_my_port(mysock));

  /* 5. Wait for the ping incomming messages */

  /** \bug if the server is gone before the forwarder tries to connect,
     it dies awfully with the following message. The problem stands somewhere
     at the interface between the xbt_socket_t and the msg mess. There is thus
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
  XBT_INFO("Done.");
  gras_exit();

  return 0;
}                               /* end_of_server */
