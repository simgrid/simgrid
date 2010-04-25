/* peer.h - peer (remote processes) management functions                    */

/* Copyright (c) 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_PEER_H
#define XBT_PEER_H

#include "xbt/misc.h"

SG_BEGIN_DECL()

     typedef struct {
       char *name;
       int port;
     } s_xbt_peer_t, *xbt_peer_t;

XBT_PUBLIC(xbt_peer_t) xbt_peer_new(const char *name, int port);
XBT_PUBLIC(xbt_peer_t) xbt_peer_from_string(const char *peerport);
XBT_PUBLIC(xbt_peer_t) xbt_peer_copy(xbt_peer_t h);
XBT_PUBLIC(void) xbt_peer_free(xbt_peer_t peer);
XBT_PUBLIC(void) xbt_peer_free_voidp(void *d);

SG_END_DECL()
#endif /* XBT_PEER_H */
