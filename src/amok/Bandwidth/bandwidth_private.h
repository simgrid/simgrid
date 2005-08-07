/* $Id$ */

/* bandwidth - network bandwidth tests facilities                           */

/* module's private interface masked even to other parts of AMOK.           */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#ifndef AMOK_BANDWIDTH_PRIVATE_H
#define AMOK_BANDWIDTH_PRIVATE_H

#include "gras.h"
#include "amok/bandwidth.h"

/* Request for a BW experiment.
 * If host==NULL, it should be between the sender and the receiver.
 * If not, it should be between between the receiver and host (3-tiers).
 */
typedef struct {
  xbt_host_t host; /* host+raw socket to use */
  unsigned long int buf_size;
  unsigned long int exp_size;
  unsigned long int msg_size;
} s_bw_request_t,*bw_request_t;

/* Result of a BW experiment (payload when answering).
 * if err.msg != NULL, it wasn't sucessful. Check err.msg and err.code to see why.
 */
typedef struct {
  unsigned int timestamp;
  double sec;
  double bw;
} s_bw_res,*bw_res_t;


/* Description of a saturation experiment (payload asking some host to collaborate for that)
 */
typedef struct {
  xbt_host_t host; /* host+raw socket to use */
  unsigned int msg_size;
  unsigned int timeout;
} s_sat_request_t,*sat_request_t;

/* Prototypes of local callbacks */
int amok_bw_cb_bw_handshake(gras_socket_t  expeditor,
			    void          *payload);
int amok_bw_cb_bw_request(gras_socket_t    expeditor,
			  void            *payload);

int amok_bw_cb_sat_start(gras_socket_t     expeditor,
			 void             *payload);
int amok_bw_cb_sat_begin(gras_socket_t     expeditor,
			 void             *payload);

#endif /* AMOK_BANDWIDTH_PRIVATE_H */
