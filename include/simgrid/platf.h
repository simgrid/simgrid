/* platf.h - Public interface to the SimGrid platforms                      */

/* Copyright (c) 2004, 2005, 2006, 2007, 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SG_PLATF_H
#define SG_PLATF_H

#include <xbt.h>                /* our toolbox */

typedef struct tmgr_trace *tmgr_trace_t; /**< Opaque structure defining an availability trace */

/** Defines whether a given resource is working or not */
typedef enum {
  SURF_RESOURCE_ON = 1,                   /**< Up & ready        */
  SURF_RESOURCE_OFF = 0                   /**< Down & broken     */
} e_surf_resource_state_t;


/*
 * Platform creation functions. Instead of passing 123 arguments to the creation functions
 * (one for each possible XML attribute), we pass structures containing them all. It removes the
 * chances of switching arguments by error, and reduce the burden when we add a new attribute:
 * old models can just continue to ignore it without having to update their headers.
 *
 * It shouldn't be too costly at runtime, provided that structures living on the stack are
 * used, instead of malloced structures.
 */

typedef struct {
  char* V_host_id;                          //id
  double V_host_power_peak;                     //power
  int V_host_core;                          //core
  double V_host_power_scale;                      //availability
  tmgr_trace_t V_host_power_trace;                  //availability file
  e_surf_resource_state_t V_host_state_initial;           //state
  tmgr_trace_t V_host_state_trace;                  //state file
  const char* V_host_coord;
} s_sg_platf_host_cbarg_t, *sg_platf_host_cbarg_t;

typedef struct {
  const char* V_router_id;
  const char* V_router_coord;
} s_sg_platf_router_cbarg_t, *sg_platf_router_cbarg_t;


XBT_PUBLIC(void) sg_platf_new_AS_open(const char *id, const char *mode);
XBT_PUBLIC(void) sg_platf_new_AS_close(void);

XBT_PUBLIC(void) sg_platf_new_host  (sg_platf_host_cbarg_t   host);
XBT_PUBLIC(void) sg_platf_new_router(sg_platf_router_cbarg_t router);


#endif                          /* SG_PLATF_H */
