/* virtu_sg - specific GRAS implementation for simulator                    */

/* Copyright (c) 2004, 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef VIRTU_SG_H
#define VIRTU_SG_H

#include "gras/Virtu/virtu_private.h"
#include "xbt/dynar.h"
#include "simix/simix.h"        /* SimGrid header */
#include "gras/Transport/transport_private.h"

typedef struct {
  int port;                     /* list of ports used by a server socket */
  int meas;                     /* (boolean) the channel is for measurements or for messages */
  smx_process_t process;
  gras_socket_t socket;
} gras_sg_portrec_t;

/* Data for each host */
typedef struct {
  int refcount;

  xbt_dynar_t ports;

} gras_hostdata_t;

/* data for each socket (FIXME: find a better location for that)*/
typedef struct {
  smx_process_t from_process;
  smx_process_t to_process;

  smx_host_t to_host;           /* Who's on other side */

  smx_cond_t cond;
  smx_mutex_t mutex;
  gras_socket_t to_socket;
} gras_trp_sg_sock_data_t;


void *gras_libdata_by_name_from_remote(const char *name, smx_process_t p);
/* The same function by id would be really dangerous.
 * 
 * Indeed, it would rely on the fact that all process register libdatas in
 * the same order, which is wrong if they init amok modules in different
 * order.
 */

#endif /* VIRTU_SG_H */
