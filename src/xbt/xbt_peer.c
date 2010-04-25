/* xbt_peer_t management functions                                          */

/* Copyright (c) 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/peer.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(peer, xbt, "peer management");

/** \brief constructor */
xbt_peer_t xbt_peer_new(const char *name, int port)
{
  xbt_peer_t res = xbt_new(s_xbt_peer_t, 1);
  res->name = xbt_strdup(name);
  res->port = port;
  return res;
}

xbt_peer_t xbt_peer_copy(xbt_peer_t h)
{
  return xbt_peer_new(h->name, h->port);
}

/** \brief constructor. Argument should be of form '(peername):(port)'. */
xbt_peer_t xbt_peer_from_string(const char *peerport)
{
  xbt_peer_t res = xbt_new(s_xbt_peer_t, 1);
  char *name = xbt_strdup(peerport);
  char *port_str = strchr(name, ':');
  xbt_assert1(port_str,
              "argument of xbt_peer_from_string should be of form <peername>:<port>, it's '%s'",
              peerport);
  *port_str = '\0';
  port_str++;

  res->name = xbt_strdup(name); /* it will be shorter now that we cut the port */
  res->port = atoi(port_str);
  free(name);
  return res;
}

/** \brief destructor */
void xbt_peer_free(xbt_peer_t peer)
{
  if (peer) {
    if (peer->name)
      free(peer->name);
    free(peer);
  }
}

/** \brief Freeing function for dynars of xbt_peer_t */
void xbt_peer_free_voidp(void *d)
{
  xbt_peer_free((xbt_peer_t) * (void **) d);
}
