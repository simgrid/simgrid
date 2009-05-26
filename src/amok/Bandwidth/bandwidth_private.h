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

void amok_bw_bw_init(void);     /* Must be called only once per node */
void amok_bw_bw_join(void);     /* Each process must run it */
void amok_bw_bw_leave(void);    /* Each process must run it */

void amok_bw_sat_init(void);    /* Must be called only once per node */
void amok_bw_sat_join(void);    /* Each process must run it */
void amok_bw_sat_leave(void);   /* Each process must run it */

/***
 * Plain bandwidth measurement stuff
 ***/

/* Request for a BW experiment.
 * If peer==NULL, it should be between the sender and the receiver.
 * If not, it should be between between the receiver and peer (3-tiers).
 */
typedef struct {
  s_xbt_peer_t peer;            /* peer+raw socket to use */
  unsigned long int buf_size;
  unsigned long int msg_size;
  unsigned long int msg_amount;
  double min_duration;
} s_bw_request_t, *bw_request_t;

/* Result of a BW experiment (payload when answering). */
typedef struct {
  unsigned int timestamp;
  double sec;
  double bw;
} s_bw_res_t, *bw_res_t;


/***
 * Saturation stuff
 ***/

/* Description of a saturation experiment (payload asking some peer to collaborate for that)
 */
typedef struct {
  s_xbt_peer_t peer;            /* peer+raw socket to use */
  unsigned int msg_size;
  unsigned int duration;
} s_sat_request_t, *sat_request_t;

void amok_bw_sat_start(const char *from_name, unsigned int from_port,
                       const char *to_name, unsigned int to_port,
                       unsigned int msg_size, unsigned int duration);

#endif /* AMOK_BANDWIDTH_PRIVATE_H */
