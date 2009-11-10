/* $Id$ */

/* virtu_sg - specific GRAS implementation for simulator                    */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

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

  /* Nothing in particular (anymore) */
} gras_hostdata_t;

/* data for each socket (FIXME: find a better location for that)*/
typedef struct {
  smx_rdv_t rdv;

  smx_process_t from_process; /* the one who created the socket */
  smx_host_t to_host;           /* Who's on other side */
  smx_comm_t comm; /* Ongoing communication */

  s_gras_msg_t ongoing_msg;
  size_t ongoing_msg_size;

  gras_socket_t to_socket; /* If != NULL, this socket was created as accept when receiving onto to_socket */
} gras_trp_sg_sock_data_t;


void *gras_libdata_by_name_from_remote(const char *name, smx_process_t p);
/* The same function by id would be really dangerous.
 * 
 * Indeed, it would rely on the fact that all process register libdatas in
 * the same order, which is wrong if they init amok modules in different
 * order.
 */

#endif /* VIRTU_SG_H */
