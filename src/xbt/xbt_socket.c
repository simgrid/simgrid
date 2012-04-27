/* transport - low level communication                                      */

/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "xbt/peer.h"
#include "xbt/dict.h"
#include "xbt/socket.h"
#include "xbt_modinter.h"
#include "portable.h"
#include "xbt_socket_private.h"
#include "gras/Msg/msg_interface.h" /* FIXME */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_trp, xbt,
                                "Conveying bytes over the network");
XBT_LOG_NEW_SUBCATEGORY(xbt_trp_meas, xbt_trp,
                        "Conveying bytes over the network without formating for perf measurements");

static short int xbt_trp_started = 0;
static xbt_dict_t xbt_trp_plugins;          /* all registered plugins */
static void xbt_trp_plugin_free(void *p);   /* free one of the plugins */

void xbt_trp_plugin_new(const char *name, xbt_trp_setup_t setup)
{
  xbt_trp_plugin_t plug = xbt_new0(s_xbt_trp_plugin_t, 1);

  XBT_DEBUG("Create plugin %s", name);

  plug->name = xbt_strdup(name);
  setup(plug);
  xbt_dict_set(xbt_trp_plugins, name, plug, NULL);
}

void xbt_trp_preinit(void)
{
  if (!xbt_trp_started) {
    /* make room for all plugins */
    xbt_trp_plugins = xbt_dict_new_homogeneous(xbt_trp_plugin_free);

#ifdef HAVE_WINSOCK2_H
    /* initialize the windows mechanism */
    {
      WORD wVersionRequested;
      WSADATA wsaData;

      wVersionRequested = MAKEWORD(2, 0);
      int res;
      res = WSAStartup(wVersionRequested, &wsaData);
      xbt_assert(res == 0, "Cannot find a usable WinSock DLL");

      /* Confirm that the WinSock DLL supports 2.0. */
      /* Note that if the DLL supports versions greater    */
      /* than 2.0 in addition to 2.0, it will still return */
      /* 2.0 in wVersion since that is the version we      */
      /* requested.                                        */

      xbt_assert(LOBYTE(wsaData.wVersion) == 2 &&
                  HIBYTE(wsaData.wVersion) == 0,
                  "Cannot find a usable WinSock DLL");
      XBT_INFO("Found and initialized winsock2");
    }                           /* The WinSock DLL is acceptable. Proceed. */
#elif HAVE_WINSOCK_H
    {
      WSADATA wsaData;
      int res;
      res = WSAStartup(0x0101, &wsaData);
      xbt_assert(res == 0, "Cannot find a usable WinSock DLL");
      XBT_INFO("Found and initialized winsock");
    }
#endif

    /* create the TCP transport plugin */
    xbt_trp_plugin_new("tcp", xbt_trp_tcp_setup);
  }

  xbt_trp_started++;
}

void xbt_trp_postexit(void)
{
  XBT_DEBUG("xbt_trp value %d", xbt_trp_started);
  if (xbt_trp_started == 0) {
    return;
  }

  if (--xbt_trp_started == 0) {
#ifdef HAVE_WINSOCK_H
    if (WSACleanup() == SOCKET_ERROR) {
      if (WSAGetLastError() == WSAEINPROGRESS) {
        WSACancelBlockingCall();
        WSACleanup();
      }
    }
#endif

    /* Delete the plugins */
    xbt_dict_free(&xbt_trp_plugins);
  }
}

void xbt_trp_plugin_free(void *p)
{
  xbt_trp_plugin_t plug = p;

  if (plug) {
    if (plug->exit) {
      plug->exit(plug);
    } else if (plug->data) {
      XBT_DEBUG("Plugin %s lacks exit(). Free data anyway.", plug->name);
      free(plug->data);
    }

    free(plug->name);
    free(plug);
  }
}


/**
 * xbt_trp_socket:
 *
 * Malloc a new socket with the TCP transport plugin and default parameters.
 */
void xbt_socket_new(int incoming, xbt_socket_t* dst)
{
  xbt_socket_new_ext(incoming, dst, xbt_trp_plugin_get_by_name("tcp"), 0, 0);
}

/**
 * xbt_trp_socket_new:
 *
 * Malloc a new socket.
 */
void xbt_socket_new_ext(int incoming,
                        xbt_socket_t * dst,
                        xbt_trp_plugin_t plugin,
                        unsigned long int buf_size,
                        int measurement)
{
  xbt_socket_t sock = xbt_new0(s_xbt_socket_t, 1);

  XBT_VERB("Create a new socket (%p)", (void *) sock);

  sock->plugin = plugin;

  sock->incoming = incoming ? 1 : 0;
  sock->outgoing = incoming ? 0 : 1;
  sock->accepting = incoming ? 1 : 0;
  sock->meas = measurement;
  sock->recvd = 0;
  sock->valid = 1;
  sock->moredata = 0;

  sock->refcount = 1;
  sock->buf_size = buf_size;
  sock->sd = -1;

  sock->data = NULL;
  sock->bufdata = NULL;

  *dst = sock;

  XBT_OUT();
}

XBT_INLINE void* xbt_socket_get_data(xbt_socket_t sock) {
  return sock->data;
}

XBT_INLINE void xbt_socket_set_data(xbt_socket_t sock, void* data) {
  sock->data = data;
}

/**
 * xbt_trp_send:
 *
 * Send a bunch of bytes from on socket
 * (stable if we know the storage will keep as is until the next trp_flush)
 */
void xbt_trp_send(xbt_socket_t sd, char *data, long int size, int stable)
{
  xbt_assert(sd->outgoing, "Socket not suited for data send");
  sd->plugin->send(sd, data, size, stable);
}

/**
 * xbt_trp_recv:
 *
 * Receive a bunch of bytes from a socket
 */
void xbt_trp_recv(xbt_socket_t sd, char *data, long int size)
{
  xbt_assert(sd->incoming, "Socket not suited for data receive");
  (sd->plugin->recv) (sd, data, size);
}

/**
 * gras_trp_flush:
 *
 * Make sure all pending communications are done
 */
void xbt_trp_flush(xbt_socket_t sd)
{
  if (sd->plugin->flush)
    (sd->plugin->flush) (sd);
}

xbt_trp_plugin_t xbt_trp_plugin_get_by_name(const char *name)
{
  return xbt_dict_get(xbt_trp_plugins, name);
}

int xbt_socket_my_port(xbt_socket_t sock)
{
  if (!sock->plugin->my_port)
    THROWF(unknown_error, 0, "Function my_port unimplemented in plugin %s",sock->plugin->name);
  return sock->plugin->my_port(sock);
}

int xbt_socket_peer_port(xbt_socket_t sock)
{
  if (!sock->plugin->peer_port)
    THROWF(unknown_error, 0, "Function peer_port unimplemented in plugin %s",sock->plugin->name);
  return sock->plugin->peer_port(sock);
}

const char *xbt_socket_peer_name(xbt_socket_t sock)
{
  xbt_assert(sock->plugin);
  return sock->plugin->peer_name(sock);
}

const char *xbt_socket_peer_proc(xbt_socket_t sock)
{
  return sock->plugin->peer_proc(sock);
}

void xbt_socket_peer_proc_set(xbt_socket_t sock, char *peer_proc)
{
  return sock->plugin->peer_proc_set(sock,peer_proc);
}

/** \brief Check if the provided socket is a measurement one (or a regular one) */
int xbt_socket_is_meas(xbt_socket_t sock)
{
  return sock->meas;
}

/** \brief Send a chunk of (random) data over a measurement socket
 *
 * @param peer measurement socket to use for the experiment
 * @param timeout timeout (in seconds)
 * @param msg_size size of each chunk sent over the socket (in bytes).
 * @param msg_amount how many of these packets you want to send.
 *
 * Calls to xbt_socket_meas_send() and xbt_socket_meas_recv() on
 * each side of the socket should be paired.
 *
 * The exchanged data is zeroed to make sure it's initialized, but
 * there is no way to control what is sent (ie, you cannot use these
 * functions to exchange data out of band).
 *
 * @warning: in SimGrid version 3.1 and previous, the numerical arguments
 *           were the total amount of data to send and the msg_size. This
 *           was changed for the fool wanting to send more than MAXINT
 *           bytes in a fat pipe.
 */
void xbt_socket_meas_send(xbt_socket_t peer,
                          unsigned int timeout,
                          unsigned long int msg_size,
                          unsigned long int msg_amount)
{
  char *chunk = NULL;
  unsigned long int sent_sofar;

  XBT_IN();
  THROWF(unknown_error, 0, "measurement sockets were broken in this release of SimGrid and should be ported back in the future."
      "If you depend on it, sorry, you have to use an older version, or wait for the future version using it...");

  if (peer->plugin == xbt_trp_plugin_get_by_name("tcp")) {
    chunk = xbt_malloc0(msg_size);
  }

  xbt_assert(peer->meas,
              "Asked to send measurement data on a regular socket");
  xbt_assert(peer->outgoing,
              "Socket not suited for data send (this is a server socket)");

  for (sent_sofar = 0; sent_sofar < msg_amount; sent_sofar++) {
    XBT_CDEBUG(xbt_trp_meas,
            "Sent %lu msgs of %lu (size of each: %lu) to %s:%d",
            sent_sofar, msg_amount, msg_size, xbt_socket_peer_name(peer),
            xbt_socket_peer_port(peer));
    peer->plugin->raw_send(peer, chunk, msg_size);
  }
  XBT_CDEBUG(xbt_trp_meas,
          "Sent %lu msgs of %lu (size of each: %lu) to %s:%d", sent_sofar,
          msg_amount, msg_size, xbt_socket_peer_name(peer),
          xbt_socket_peer_port(peer));

  if (peer->plugin == xbt_trp_plugin_get_by_name("tcp")) {
    free(chunk);
  }

  XBT_OUT();
}

/** \brief Receive a chunk of data over a measurement socket
 *
 * Calls to xbt_socket_meas_send() and xbt_socket_meas_recv() on
 * each side of the socket should be paired.
 *
 * @warning: in SimGrid version 3.1 and previous, the numerical arguments
 *           were the total amount of data to send and the msg_size. This
 *           was changed for the fool wanting to send more than MAXINT
 *           bytes in a fat pipe.
 */
void xbt_socket_meas_recv(xbt_socket_t peer,
                           unsigned int timeout,
                           unsigned long int msg_size,
                           unsigned long int msg_amount)
{

  char *chunk = NULL;
  unsigned long int got_sofar;

  XBT_IN();
  THROWF(unknown_error,0,"measurement sockets were broken in this release of SimGrid and should be ported back in the future."
      "If you depend on it, sorry, you have to use an older version, or wait for the future version using it...");

  if (peer->plugin == xbt_trp_plugin_get_by_name("tcp")) {
    chunk = xbt_malloc(msg_size);
  }

  xbt_assert(peer->meas,
              "Asked to receive measurement data on a regular socket");
  xbt_assert(peer->incoming, "Socket not suited for data receive");

  for (got_sofar = 0; got_sofar < msg_amount; got_sofar++) {
    XBT_CDEBUG(xbt_trp_meas,
            "Recvd %lu msgs of %lu (size of each: %lu) from %s:%d",
            got_sofar, msg_amount, msg_size, xbt_socket_peer_name(peer),
            xbt_socket_peer_port(peer));
    (peer->plugin->raw_recv) (peer, chunk, msg_size);
  }
  XBT_CDEBUG(xbt_trp_meas,
          "Recvd %lu msgs of %lu (size of each: %lu) from %s:%d",
          got_sofar, msg_amount, msg_size, xbt_socket_peer_name(peer),
          xbt_socket_peer_port(peer));

  if (peer->plugin == xbt_trp_plugin_get_by_name("tcp")) {
    free(chunk);
  }

  XBT_OUT();
}

/**
 * \brief Something similar to the good old accept system call.
 *
 * Make sure that there is someone speaking to the provided server socket.
 * In RL, it does an accept(2) and return the result as last argument.
 * In SG, as accepts are useless, it returns the provided argument as result.
 * You should thus test whether (peer != accepted) before closing both of them.
 *
 * You should only call this on measurement sockets. It is automatically
 * done for regular sockets, but you usually want more control about
 * what's going on with measurement sockets.
 */
xbt_socket_t xbt_socket_meas_accept(xbt_socket_t peer)
{
  xbt_socket_t res;
  THROWF(unknown_error,0,"measurement sockets were broken in this release of SimGrid and should be ported back in the future."
      "If you depend on it, sorry, you have to use an older version, or wait for the future version using it...");

  xbt_assert(peer->meas,
              "No need to accept on non-measurement sockets (it's automatic)");

  if (!peer->accepting) {
    /* nothing to accept here (must be in SG) */
    /* BUG: FIXME: this is BAD! it makes tricky to free the accepted socket */
    return peer;
  }

  res = (peer->plugin->socket_accept) (peer);
  res->meas = peer->meas;
  XBT_CDEBUG(xbt_trp_meas, "meas_accepted onto %d", res->sd);

  return res;
}
