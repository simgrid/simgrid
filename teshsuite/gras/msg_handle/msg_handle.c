/* $Id: mmrpc.c 3399 2007-04-11 19:34:43Z cherierm $ */

/* msg_handle - ensures the semantic of gras_msg_handle(i) for i<0,=0 or >0 */

/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */
/* Thanks to Flavien Vernier for reporting an issue around this             */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Messages specific to this test");

int server(int argc, char *argv[]);
int client(int argc, char *argv[]);

static int server_cb_hello_handler(gras_msg_cb_ctx_t ctx,
                                   void *payload_data)
{
  INFO0("Got the message");
  return 0;
}

int server(int argc, char *argv[])
{
  gras_socket_t me = NULL, pal = NULL;
  int myport;
  char *palstr;

  xbt_ex_t e;
  int got_expected;
  double now;


  gras_init(&argc, argv);

  xbt_assert0(argc == 3, "Usage: server <myport> <client>");
  myport = atoi(argv[1]);
  palstr = argv[2];

  gras_msgtype_declare("hello", NULL);
  gras_cb_register("hello", &server_cb_hello_handler);

  INFO1("Launch server (port=%d)", myport);
  TRY {
    me = gras_socket_server(myport);
  } CATCH(e) {
    RETHROW0("Unable to establish a server socket: %s");
  }
  gras_os_sleep(1);             /* Wait for pal to startup */
  TRY {
    pal = gras_socket_client_from_string(palstr);
  } CATCH(e) {
    RETHROW1("Unable to establish a socket to %s: %s", palstr);
  }
  INFO0("Initialization done.");
  now = gras_os_time();

  /* Launch handle(0) when there is no message. Timeout expected */
  got_expected = 0;
  TRY {
    gras_msg_handle(0);
  } CATCH(e) {
    if (e.category == timeout_error) {
      got_expected = 1;
      xbt_ex_free(e);
    } else {
      RETHROW0("Didn't got the expected timeout: %s");
    }
  }
  xbt_assert0(got_expected,
              "gras_msg_handle(0) do not lead to any timeout exception");
  xbt_assert1(gras_os_time() - now < 0.01,
              "gras_msg_handle(0) do not anwser immediately (%.4fsec)",
              gras_os_time() - now);
  INFO0("gras_msg_handle(0) works as expected (immediate timeout)");
  /* Launch handle(0) when there is no message. Timeout expected */
  got_expected = 0;
  TRY {
    gras_msg_handle(1);
  }
  CATCH(e) {
    if (e.category == timeout_error) {
      got_expected = 1;
      xbt_ex_free(e);
    } else {
      RETHROW0("Didn't got the expected timeout: %s");
    }
  }
  xbt_assert0(got_expected,
              "gras_msg_handle(1) do not lead to any timeout exception");
  xbt_assert1(gras_os_time() - now < 1.5,
              "gras_msg_handle(1) needs more than 1.5 sec to answer (%.4fsec)",
              gras_os_time() - now);
  xbt_assert1(gras_os_time() - now >= 1.0,
              "gras_msg_handle(1) answers in less than one second (%.4fsec)",
              gras_os_time() - now);
  INFO0("gras_msg_handle(1) works as expected (delayed timeout)");
  gras_os_sleep(3);

  /* Send an hello to the client to unlock it */
  INFO0("Unlock pal");
  gras_msg_send(pal, "hello", NULL);

  /* Frees the allocated resources, and shut GRAS down */
  gras_socket_close(me);
  gras_socket_close(pal);
  gras_exit();
  return 0;
}

int client(int argc, char *argv[])
{
  gras_socket_t me = NULL, pal = NULL;
  int myport;
  char *palstr;

  xbt_ex_t e;
  int got_expected;


  gras_init(&argc, argv);
  xbt_assert0(argc == 3, "Usage: client <myport> <server>");
  myport = atoi(argv[1]);
  palstr = argv[2];

  gras_msgtype_declare("hello", NULL);
  gras_cb_register("hello", &server_cb_hello_handler);

  INFO1("Launch client (port=%d)", myport);
  TRY {
    me = gras_socket_server(myport);
  } CATCH(e) {
    RETHROW0("Unable to establish a server socket: %s");
  }
  gras_os_sleep(1);             /* Wait for pal to startup */
  TRY {
    pal = gras_socket_client_from_string(palstr);
  } CATCH(e) {
    RETHROW1("Unable to establish a socket to %s: %s", palstr);
  }
  INFO0("Initialization done.");

  /* Launch handle(-1). Lock until message from server expected */
  got_expected = 0;
  TRY {
    gras_msg_handle(-1);
  } CATCH(e) {
    RETHROW0("No exception expected during handle(-1), but got %s");
  }
  INFO0("gras_msg_handle(-1) works as expected (locked)");

  /* Frees the allocated resources, and shut GRAS down */
  gras_socket_close(me);
  gras_socket_close(pal);
  gras_exit();
  return 0;
}
