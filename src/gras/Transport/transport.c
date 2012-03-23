/* transport - low level communication                                      */

/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/***
 *** Options
 ***/
#ifndef NDEBUG
static int gras_opt_trp_nomoredata_on_close = 0;
#endif

#include "xbt/ex.h"
#include "xbt/peer.h"
#include "xbt/socket.h"
#include "xbt/xbt_socket_private.h" /* FIXME */
#include "portable.h"
#include "gras/Transport/transport_private.h"
#include "gras/Msg/msg_interface.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras_trp, gras,
                                "Conveying bytes over the network");

/**
 * @brief Opens a server socket and makes it ready to be listened to.
 * @param port: port on which you want to listen
 * @param buf_size: size of the buffer (in byte) on the socket (for TCP sockets only). If 0, a sain default is used (32k, but may change)
 * @param measurement: whether this socket is meant to convey measurement (if you don't know, use 0 to exchange regular messages)
 *
 * In real life, you'll get a TCP socket.
 */
xbt_socket_t
gras_socket_server_ext(unsigned short port,
                       unsigned long int buf_size, int measurement)
{
  xbt_trp_plugin_t trp;
  xbt_socket_t sock;

  XBT_DEBUG("Create a server socket from plugin %s on port %d",
         gras_if_RL() ? "tcp" : "sg", port);
  trp = xbt_trp_plugin_get_by_name(gras_if_SG() ? "sg" : "tcp");

  /* defaults settings */
  xbt_socket_new_ext(1,
                     &sock,
                     trp,
                     (buf_size > 0 ? buf_size : 32 * 1024),
                     measurement);

  /* Call plugin socket creation function */
  XBT_DEBUG("Prepare socket with plugin (fct=%p)", trp->socket_server);
  TRY {
    trp->socket_server(trp, port, sock);
  }
  CATCH_ANONYMOUS {
    free(sock);
    RETHROW;
  }

  if (!measurement)
    ((gras_trp_procdata_t) gras_libdata_by_id(gras_trp_libdata_id))->myport
        = port;
  xbt_dynar_push(((gras_trp_procdata_t)
                  gras_libdata_by_id(gras_trp_libdata_id))->sockets,
                 &sock);

  gras_msg_listener_awake();
  return sock;
}

/**
 * @brief Opens a server socket on any port in the given range
 *
 * @param minport: first port we will try
 * @param maxport: last port we will try
 * @param buf_size: size of the buffer (in byte) on the socket (for TCP sockets only). If 0, a sain default is used (32k, but may change)
 * @param measurement: whether this socket is meant to convey measurement (if you don't know, use 0 to exchange regular messages)
 *
 * If none of the provided ports works, raises the exception got when trying the last possibility
 */
xbt_socket_t
gras_socket_server_range(unsigned short minport, unsigned short maxport,
                         unsigned long int buf_size, int measurement)
{

  int port;
  xbt_socket_t res = NULL;
  xbt_ex_t e;

  for (port = minport; port < maxport; port++) {
    TRY {
      res = gras_socket_server_ext(port, buf_size, measurement);
    }
    CATCH(e) {
      if (port == maxport)
        RETHROW;
      xbt_ex_free(e);
    }
    if (res)
      return res;
  }
  THROW_IMPOSSIBLE;
}

/**
 * @brief Opens a client socket to a remote host.
 * @param host: who you want to connect to
 * @param port: where you want to connect to on this host
 * @param buf_size: size of the buffer (in bytes) on the socket (for TCP sockets only). If 0, a sain default is used (32k, but may change)
 * @param measurement: whether this socket is meant to convey measurement (if you don't know, use 0 to exchange regular messages)
 *
 * In real life, you'll get a TCP socket.
 */
xbt_socket_t
gras_socket_client_ext(const char *host,
                       unsigned short port,
                       unsigned long int buf_size, int measurement)
{
  xbt_trp_plugin_t trp;
  xbt_socket_t sock;

  trp = xbt_trp_plugin_get_by_name(gras_if_SG() ? "sg" : "tcp");

  XBT_DEBUG("Create a client socket from plugin %s",
         gras_if_RL()? "tcp" : "sg");
  /* defaults settings */
  xbt_socket_new_ext(0,
                     &sock,
                     trp,
                     (buf_size > 0 ? buf_size : 32 * 1024),
                     measurement);

  /* plugin-specific */
  TRY {
    (*trp->socket_client) (trp, host, port, sock);
  }
  CATCH_ANONYMOUS {
    free(sock);
    RETHROW;
  }
  xbt_dynar_push(((gras_trp_procdata_t)
                  gras_libdata_by_id(gras_trp_libdata_id))->sockets,
                 &sock);
  gras_msg_listener_awake();
  return sock;
}

/**
 * @brief Opens a server socket and make it ready to be listened to.
 *
 * In real life, you'll get a TCP socket.
 */
xbt_socket_t gras_socket_server(unsigned short port)
{
  return gras_socket_server_ext(port, 32 * 1024, 0);
}

/** @brief Opens a client socket to a remote host */
xbt_socket_t gras_socket_client(const char *host, unsigned short port)
{
  return gras_socket_client_ext(host, port, 0, 0);
}

/** @brief Opens a client socket to a remote host specified as '\a host:\a port' */
xbt_socket_t gras_socket_client_from_string(const char *host)
{
  xbt_peer_t p = xbt_peer_from_string(host);
  xbt_socket_t res = gras_socket_client_ext(p->name, p->port, 0, 0);
  xbt_peer_free(p);
  return res;
}

void gras_socket_close_voidp(void *sock)
{
  gras_socket_close((xbt_socket_t) sock);
}

/** \brief Close socket */
void gras_socket_close(xbt_socket_t sock)
{
  if (--sock->refcount)
    return;

  xbt_dynar_t sockets =
      ((gras_trp_procdata_t)
       gras_libdata_by_id(gras_trp_libdata_id))->sockets;
  xbt_socket_t sock_iter = NULL;
  unsigned int cursor;

  XBT_IN("");
  XBT_VERB("Close %p", sock);
  if (sock == _gras_lastly_selected_socket) {
    xbt_assert(!gras_opt_trp_nomoredata_on_close || !sock->moredata,
                "Closing a socket having more data in buffer while the nomoredata_on_close option is activated");

    if (sock->moredata)
      XBT_CRITICAL
          ("Closing a socket having more data in buffer. Option nomoredata_on_close disabled, so continuing.");
    _gras_lastly_selected_socket = NULL;
  }

  /* FIXME: Issue an event when the socket is closed */
  XBT_DEBUG("sockets pointer before %p", sockets);
  if (sock) {
    /* FIXME: Cannot get the dynar mutex, because it can be already locked */
//              _xbt_dynar_foreach(sockets,cursor,sock_iter) {
    for (cursor = 0; cursor < xbt_dynar_length(sockets); cursor++) {
      _xbt_dynar_cursor_get(sockets, cursor, &sock_iter);
      if (sock == sock_iter) {
        XBT_DEBUG("remove sock cursor %u dize %lu\n", cursor,
               xbt_dynar_length(sockets));
        xbt_dynar_cursor_rm(sockets, &cursor);
        if (sock->plugin->socket_close)
          (*sock->plugin->socket_close) (sock);

        /* free the memory */
        free(sock);
        XBT_OUT();
        return;
      }
    }
    XBT_WARN
        ("Ignoring request to free an unknown socket (%p). Execution stack:",
         sock);
    xbt_backtrace_display_current();
  }
  XBT_OUT();
}

/*
 * Creating procdata for this module
 */
static void *gras_trp_procdata_new(void)
{
  gras_trp_procdata_t res = xbt_new(s_gras_trp_procdata_t, 1);

  res->name = xbt_strdup("gras_trp");
  res->name_len = 0;
  res->sockets = xbt_dynar_new_sync(sizeof(xbt_socket_t *), NULL);
  res->myport = 0;

  return (void *) res;
}

/*
 * Freeing procdata for this module
 */
static void gras_trp_procdata_free(void *data)
{
  gras_trp_procdata_t res = (gras_trp_procdata_t) data;

  xbt_dynar_free(&(res->sockets));
  free(res->name);
  free(res);
}

void gras_trp_socketset_dump(const char *name)
{
  gras_trp_procdata_t procdata =
      (gras_trp_procdata_t) gras_libdata_by_id(gras_trp_libdata_id);

  unsigned int it;
  xbt_socket_t s;

  XBT_INFO("** Dump the socket set %s", name);
  xbt_dynar_foreach(procdata->sockets, it, s) {
    XBT_INFO("  %p -> %s:%d %s",
          s, xbt_socket_peer_name(s), xbt_socket_peer_port(s),
          s->valid ? "(valid)" : "(peer dead)");
  }
  XBT_INFO("** End of socket set %s", name);
}

/*
 * Module registration
 */
int gras_trp_libdata_id;
void gras_trp_register()
{
  gras_trp_libdata_id =
      gras_procdata_add("gras_trp", gras_trp_procdata_new,
                        gras_trp_procdata_free);
}

int gras_os_myport(void)
{
  return ((gras_trp_procdata_t)
          gras_libdata_by_id(gras_trp_libdata_id))->myport;
}
