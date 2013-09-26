/* peer.h - peer (remote processes) management functions                    */

/* Copyright (c) 2006-2007, 2009-2010, 2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_PEER_H
#define XBT_PEER_H

#include "xbt/misc.h"

SG_BEGIN_DECL()

/** @addtogroup XBT_peer
 *  \brief Helper functions to manipulate remote hosts
 *
 *  This module simply introduces some rather trivial functions to manipulate remote host denomination (in the form hostname:port)
 *
 *  @{
 */
/** @brief Object describing a remote host (as name:port) */
typedef struct s_xbt_peer *xbt_peer_t;

/** @brief Structure describing a remote host (as name:port) */
typedef struct s_xbt_peer {
  char *name;
  int port;
} s_xbt_peer_t;

XBT_PUBLIC(xbt_peer_t) xbt_peer_new(const char *name, int port);
XBT_PUBLIC(xbt_peer_t) xbt_peer_from_string(const char *peerport);
XBT_PUBLIC(xbt_peer_t) xbt_peer_copy(xbt_peer_t h);
XBT_PUBLIC(void) xbt_peer_free(xbt_peer_t peer);
XBT_PUBLIC(void) xbt_peer_free_voidp(void *d);

/** @} */
SG_END_DECL()
#endif                          /* XBT_PEER_H */
