/* $Id$ */

/* virtu_sg - specific GRAS implementation for simulator                    */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003,2004 da GRAS posse.                                   */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef VIRTU_SG_H
#define VIRTU_SG_H

#include "gras_private.h"
#include "Virtu/virtu_interface.h"
#include <msg.h> /* SimGrid header */

#define GRAS_MAX_CHANNEL 10

typedef struct {
  int port;  /* list of ports used by a server socket */
  int tochan; /* the channel it points to */
  int raw;   /* (boolean) the channel is in raw mode or for messages */
} gras_sg_portrec_t;

/* Data for each host */
typedef struct {
  int proc[GRAS_MAX_CHANNEL]; /* PID of who's connected to each channel */
                              /* If =0, then free */

  gras_dynar_t *ports;

} gras_hostdata_t;

/* data for each socket (FIXME: find a better location for that)*/
typedef struct {
  int from_PID;    /* process which sent this message */
  int to_PID;      /* process to which this message is destinated */

  m_host_t to_host;   /* Who's on other side */
  m_channel_t to_chan;/* Channel on which the other side is earing */
} gras_trp_sg_sock_data_t;


#endif /* VIRTU_SG_H */
