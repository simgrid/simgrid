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

/***
 *** Options
 ***/
extern int gras_opt_trp_nomoredata_on_close;

/***
 *** Main user functions
 ***/
/* stable if we know the storage will keep as is until the next trp_flush */
XBT_PUBLIC(void) gras_trp_send(gras_socket_t sd, char *data, long int size,
                               int stable);
XBT_PUBLIC(void) gras_trp_recv(gras_socket_t sd, char *data,
                               long int size);
XBT_PUBLIC(void) gras_trp_flush(gras_socket_t sd);

/* Find which socket needs to be read next */
XBT_PUBLIC(gras_socket_t) gras_trp_select(double timeout);

/* Set the peer process name (used by messaging layer) */
XBT_PUBLIC(void) gras_socket_peer_proc_set(gras_socket_t sock,
                                           char *peer_proc);

/***
 *** Plugin mechanism 
 ***/

/* A plugin type */
typedef struct gras_trp_plugin_ s_gras_trp_plugin_t, *gras_trp_plugin_t;

/* A plugin type */
struct gras_trp_plugin_ {
  char *name;

  /* dst pointers are created and initialized with default values
     before call to socket_client/server. 
     Retrive the info you need from there. */
  void (*socket_client) (gras_trp_plugin_t self, gras_socket_t dst);
  void (*socket_server) (gras_trp_plugin_t self, gras_socket_t dst);

  gras_socket_t(*socket_accept) (gras_socket_t from);


  /* socket_close() is responsible of telling the OS that the socket is over,
     but should not free the socket itself (beside the specific part) */
  void (*socket_close) (gras_socket_t sd);

  /* send/recv may be buffered */
  void (*send) (gras_socket_t sd,
                const char *data,
                unsigned long int size,
                int stable /* storage will survive until flush */ );
  int (*recv) (gras_socket_t sd, char *data, unsigned long int size);
  /* raw_send/raw_recv is never buffered (use it for measurement stuff) */
  void (*raw_send) (gras_socket_t sd,
                    const char *data, unsigned long int size);
  int (*raw_recv) (gras_socket_t sd, char *data, unsigned long int size);

  /* flush has to make sure that the pending communications are achieved */
  void (*flush) (gras_socket_t sd);

  void *data;                   /* plugin-specific data */

  /* exit is responsible for freeing data and telling to the OS that 
     this plugin is gone */
  /* exit=NULL, data gets brutally free()d by the generic interface. 
     (ie exit function needed only when data contains pointers) */
  void (*exit) (gras_trp_plugin_t);
};

XBT_PUBLIC(gras_trp_plugin_t)
    gras_trp_plugin_get_by_name(const char *name);

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

#endif                          /* GRAS_TRP_INTERFACE_H */
