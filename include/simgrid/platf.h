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
  xbt_dict_t properties;
} s_sg_platf_host_cbarg_t, *sg_platf_host_cbarg_t;

typedef struct {
  const char* V_router_id;
  const char* V_router_coord;
} s_sg_platf_router_cbarg_t, *sg_platf_router_cbarg_t;

typedef struct {
  char* V_link_id;
  double V_link_bandwidth;
  tmgr_trace_t V_link_bandwidth_file;
  double V_link_latency;
  tmgr_trace_t V_link_latency_file;
  e_surf_resource_state_t V_link_state;
  tmgr_trace_t V_link_state_file;
  int V_link_sharing_policy;
  int V_policy_initial_link;
  xbt_dict_t properties;
} s_sg_platf_link_cbarg_t, *sg_platf_link_cbarg_t;


XBT_PUBLIC(void) sg_platf_open(void);  // Start a new platform
XBT_PUBLIC(void) sg_platf_close(void); // Finish the creation of the platform

XBT_PUBLIC(void) sg_platf_new_AS_open(const char *id, const char *mode); // Begin description of new AS
XBT_PUBLIC(void) sg_platf_new_AS_close(void);                            // That AS is fully described

XBT_PUBLIC(void) sg_platf_new_host  (sg_platf_host_cbarg_t   host);   // Add an host  to the currently described AS
XBT_PUBLIC(void) sg_platf_new_router(sg_platf_router_cbarg_t router); // Add a router to the currently described AS
XBT_PUBLIC(void) sg_platf_new_link(sg_platf_link_cbarg_t link);       // Add a link   to the currently described AS


#endif                          /* SG_PLATF_H */
