/* transport - low level communication (send/receive bunches of bytes)      */

/* module's private interface masked even to other parts of GRAS.           */

/* Copyright (c) 2004, 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_TRP_PRIVATE_H
#define GRAS_TRP_PRIVATE_H

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"

#include "gras/emul.h"          /* gras_if_RL() */

#include "gras_modinter.h"      /* module init/exit */
#include "gras/transport.h"     /* rest of module interface */

#include "gras/Transport/transport_interface.h" /* semi-public API */
#include "gras/Virtu/virtu_interface.h" /* libdata management */

extern int gras_trp_libdata_id; /* our libdata identifier */

/* The function that select returned the last time we asked. We need this
   because the TCP read are greedy and try to get as much data in their 
   buffer as possible (to avoid subsequent syscalls).
   (measurement sockets are not buffered and thus not concerned).
  
   So, we can get more than one message in one shoot. And when this happens,
   we have to handle the same socket again afterward without select()ing at
   all. 
 
   Then, this data is not a static of the TCP driver because we want to
   zero it when it gets closed by the user. If not, we use an already freed 
   pointer, which is bad.
 
   It gets tricky since gras_socket_close is part of the common API, not 
   only the RL one. */
extern xbt_socket_t _gras_lastly_selected_socket;

#endif                          /* GRAS_TRP_PRIVATE_H */
