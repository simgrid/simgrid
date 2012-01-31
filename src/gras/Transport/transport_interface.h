/* transport - low level communication (send/receive bunches of bytes)      */

/* module's public interface exported within GRAS, but not to end user.     */

/* Copyright (c) 2004, 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_TRP_INTERFACE_H
#define GRAS_TRP_INTERFACE_H

#include "portable.h"           /* sometimes needed for fd_set */
#include "simix/simix.h"
#include "xbt/queue.h"

/* Data of this module specific to each process
 * (used by sg_process.c to cleanup the SG channel cruft)
 */
typedef struct {
  /* set headers */
  unsigned int ID;
  char *name;
  unsigned int name_len;

  int myport;                   /* Port on which I listen myself */

  xbt_dynar_t sockets;          /* all sockets known to this process */

  /* SG only elements. In RL, they are part of the OS ;) */

  /* List of sockets ready to be select()ed */
  xbt_queue_t msg_selectable_sockets;   /* regular sockets  */
  xbt_queue_t meas_selectable_sockets;  /* measurement ones */

} s_gras_trp_procdata_t, *gras_trp_procdata_t;

/* Display the content of our socket set (debugging purpose) */
XBT_PUBLIC(void) gras_trp_socketset_dump(const char *name);

XBT_PUBLIC(xbt_socket_t) gras_trp_select(double timeout);

#endif                          /* GRAS_TRP_INTERFACE_H */
