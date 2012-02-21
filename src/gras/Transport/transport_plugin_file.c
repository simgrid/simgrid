/* File transport - send/receive a bunch of bytes from a file               */

/* Copyright (c) 2004, 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "portable.h"
#include "gras/Transport/transport_private.h"
#include "xbt/ex.h"
#include "xbt/xbt_socket_private.h"
#include "gras/Msg/msg_interface.h"     /* gras_msg_listener_awake */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras_trp_file, gras_trp,
                                "Pseudo-transport to write to/read from a file");

/***
 *** Prototypes
 ***/
void gras_trp_file_close(xbt_socket_t sd);

void gras_trp_file_chunk_send_raw(xbt_socket_t sd,
                                  const char *data,
                                  unsigned long int size);
void gras_trp_file_chunk_send(xbt_socket_t sd, const char *data,
                              unsigned long int size, int stable_ignored);

int gras_trp_file_chunk_recv(xbt_socket_t sd,
                             char *data, unsigned long int size);

/***
 *** Specific plugin part
 ***/

typedef struct {
  fd_set incoming_socks;
} gras_trp_file_plug_data_t;

/***
 *** Specific socket part
 ***/

/***
 *** Info about who's speaking
 ***/
static int gras_trp_file_my_port(xbt_socket_t s) {
  THROW_UNIMPLEMENTED;
}
static int gras_trp_file_peer_port(xbt_socket_t s) {
  THROW_UNIMPLEMENTED;
}
static const char* gras_trp_file_peer_name(xbt_socket_t s) {
  THROW_UNIMPLEMENTED;
}
static const char* gras_trp_file_peer_proc(xbt_socket_t s) {
  THROW_UNIMPLEMENTED;
}
static void gras_trp_file_peer_proc_set(xbt_socket_t s,char *name) {
  THROW_UNIMPLEMENTED;
}

/***
 *** Code
 ***/
void gras_trp_file_setup(xbt_trp_plugin_t plug)
{

  gras_trp_file_plug_data_t *file = xbt_new(gras_trp_file_plug_data_t, 1);

  FD_ZERO(&(file->incoming_socks));

  plug->my_port = gras_trp_file_my_port;
  plug->peer_port = gras_trp_file_peer_port;
  plug->peer_name = gras_trp_file_peer_name;
  plug->peer_proc = gras_trp_file_peer_proc;
  plug->peer_proc_set = gras_trp_file_peer_proc_set;

  plug->socket_close = gras_trp_file_close;

  plug->raw_send = gras_trp_file_chunk_send_raw;
  plug->send = gras_trp_file_chunk_send;

  plug->raw_recv = plug->recv = gras_trp_file_chunk_recv;

  plug->data = (void *) file;
}

/**
 * gras_socket_client_from_file:
 *
 * Create a client socket from a file path.
 *
 * This only possible in RL, and is mainly for debugging.
 */
xbt_socket_t gras_socket_client_from_file(const char *path)
{
  xbt_socket_t res;

  xbt_assert(gras_if_RL(), "Cannot use file as socket in the simulator");

  xbt_socket_new_ext(0, &res, xbt_trp_plugin_get_by_name("file"), 0, 0);

  if (strcmp("-", path)) {
    res->sd =
        open(path, O_TRUNC | O_WRONLY | O_CREAT | O_BINARY,
             S_IRUSR | S_IWUSR | S_IRGRP);

    if (res->sd < 0) {
      THROWF(system_error, 0,
             "Cannot create a client socket from file %s: %s",
             path, strerror(errno));
    }
  } else {
    res->sd = 1;                /* stdout */
  }

  XBT_DEBUG("sock_client_from_file(%s): sd=%d in=%c out=%c accept=%c",
         path,
         res->sd,
         res->incoming ? 'y' : 'n',
         res->outgoing ? 'y' : 'n', res->accepting ? 'y' : 'n');

  xbt_dynar_push(((gras_trp_procdata_t)
                  gras_libdata_by_id(gras_trp_libdata_id))->sockets, &res);
  return res;
}

/**
 * gras_socket_server_from_file:
 *
 * Create a server socket from a file path.
 *
 * This only possible in RL, and is mainly for debugging.
 */
xbt_socket_t gras_socket_server_from_file(const char *path)
{
  xbt_socket_t res;

  xbt_assert(gras_if_RL(), "Cannot use file as socket in the simulator");

  xbt_socket_new_ext(1, &res, xbt_trp_plugin_get_by_name("file"), 0, 0);

  if (strcmp("-", path)) {
    res->sd = open(path, O_RDONLY | O_BINARY);

    if (res->sd < 0) {
      THROWF(system_error, 0,
             "Cannot create a server socket from file %s: %s",
             path, strerror(errno));
    }
  } else {
    res->sd = 0;                /* stdin */
  }

  XBT_DEBUG("sd=%d in=%c out=%c accept=%c",
         res->sd,
         res->incoming ? 'y' : 'n',
         res->outgoing ? 'y' : 'n', res->accepting ? 'y' : 'n');

  xbt_dynar_push(((gras_trp_procdata_t)
                  gras_libdata_by_id(gras_trp_libdata_id))->sockets, &res);
  gras_msg_listener_awake();
  return res;
}

void gras_trp_file_close(xbt_socket_t sock)
{
  gras_trp_file_plug_data_t *data;

  if (!sock)
    return;                     /* close only once */
  data = sock->plugin->data;

  if (sock->sd == 0) {
    XBT_DEBUG("Do not close stdin");
  } else if (sock->sd == 1) {
    XBT_DEBUG("Do not close stdout");
  } else {
    XBT_DEBUG("close file connection %d", sock->sd);

    /* forget about the socket */
    FD_CLR(sock->sd, &(data->incoming_socks));

    /* close the socket */
    if (close(sock->sd) < 0) {
      XBT_WARN("error while closing file %d: %s", sock->sd, strerror(errno));
    }
  }
}

/**
 * gras_trp_file_chunk_send:
 *
 * Send data on a file pseudo-socket
 */
void
gras_trp_file_chunk_send(xbt_socket_t sock,
                         const char *data,
                         unsigned long int size, int stable_ignored)
{
  gras_trp_file_chunk_send_raw(sock, data, size);
}

void
gras_trp_file_chunk_send_raw(xbt_socket_t sock,
                             const char *data, unsigned long int size)
{

  xbt_assert(sock->outgoing, "Cannot write on client file socket");

  while (size) {
    int status = 0;

    XBT_DEBUG("write(%d, %p, %ld);", sock->sd, data, (long int) size);
    status = write(sock->sd, data, (long int) size);

    if (status == -1) {
      THROWF(system_error, 0, "write(%d,%p,%d) failed: %s",
             sock->sd, data, (int) size, strerror(errno));
    }

    if (status) {
      size -= status;
      data += status;
    } else {
      THROWF(system_error, 0, "file descriptor closed");
    }
  }
}

/**
 * gras_trp_file_chunk_recv:
 *
 * Receive data on a file pseudo-socket.
 */
int
gras_trp_file_chunk_recv(xbt_socket_t sock,
                         char *data, unsigned long int size)
{

  int got = 0;

  xbt_assert(sock, "Cannot recv on an NULL socket");
  xbt_assert(sock->incoming, "Cannot recv on client file socket");

  if (sock->recvd) {
    data[0] = sock->recvd_val;
    sock->recvd = 0;
    got++;
    size--;
  }

  while (size) {
    int status = 0;

    status = read(sock->sd, data + got, (long int) size);
    XBT_DEBUG("read(%d, %p, %ld);", sock->sd, data + got, size);

    if (status < 0) {
      THROWF(system_error, 0, "read(%d,%p,%d) failed: %s",
             sock->sd, data + got, (int) size, strerror(errno));
    }

    if (status) {
      size -= status;
      got += status;
    } else {
      THROWF(system_error, errno, "file descriptor closed after %d bytes",
             got);
    }
  }
  return got;
}
