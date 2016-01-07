/* platf.h - Public interface to the SimGrid platforms                      */

/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SG_PLATF_H
#define SG_PLATF_H

#include <xbt.h>
#include <simgrid/host.h>
#include "forward.h"

SG_BEGIN_DECL()

static inline char* sg_storage_name(sg_storage_t storage) {
  return storage->key;
}

XBT_PUBLIC(sg_netcard_t) sg_netcard_by_name_or_null(const char *name);

XBT_PUBLIC(tmgr_trace_t) tmgr_trace_new_from_file(const char *filename);
XBT_PUBLIC(tmgr_trace_t) tmgr_trace_new_from_string(const char *id,
                                                    const char *input,
                                                    double periodicity);

XBT_PUBLIC(tmgr_trace_t) tmgr_trace_generator_value(const char *id,
                                              probabilist_event_generator_t date_generator,
                                              probabilist_event_generator_t value_generator);
XBT_PUBLIC(tmgr_trace_t) tmgr_trace_generator_state(const char *id,
                                              probabilist_event_generator_t date_generator,
                                              int first_event_hostIsOn);
XBT_PUBLIC(tmgr_trace_t) tmgr_trace_generator_avail_unavail(const char *id,
                                              probabilist_event_generator_t avail_duration_generator,
                                              probabilist_event_generator_t unavail_duration_generator,
                                              int first_event_hostIsOn);

XBT_PUBLIC(probabilist_event_generator_t) tmgr_event_generator_new_uniform(const char* id,
                                                                           double min,
                                                                           double max);
XBT_PUBLIC(probabilist_event_generator_t) tmgr_event_generator_new_exponential(const char* id,
                                                                           double rate);
XBT_PUBLIC(probabilist_event_generator_t) tmgr_event_generator_new_weibull(const char* id,
                                                                           double scale,
                                                                           double shape);

/* ***************************************** */

XBT_PUBLIC(void) sg_platf_begin(void);  // Start a new platform
XBT_PUBLIC(void) sg_platf_end(void); // Finish the creation of the platform

XBT_PUBLIC(void) sg_platf_new_AS_begin(sg_platf_AS_cbarg_t AS); // Begin description of new AS
XBT_PUBLIC(void) sg_platf_new_AS_end(void);                            // That AS is fully described

XBT_PUBLIC(void) sg_platf_new_host   (sg_platf_host_cbarg_t   host);   // Add an host   to the currently described AS
XBT_PUBLIC(void) sg_platf_new_netcard(sg_platf_host_link_cbarg_t h); // Add an host_link to the currently described AS
XBT_PUBLIC(void) sg_platf_new_router (sg_platf_router_cbarg_t router); // Add a router  to the currently described AS
XBT_PUBLIC(void) sg_platf_new_link   (sg_platf_link_cbarg_t link);     // Add a link    to the currently described AS
XBT_PUBLIC(void) sg_platf_new_peer   (sg_platf_peer_cbarg_t peer);     // Add a peer    to the currently described AS
XBT_PUBLIC(void) sg_platf_new_cluster(sg_platf_cluster_cbarg_t clust); // Add a cluster to the currently described AS
XBT_PUBLIC(void) sg_platf_new_cabinet(sg_platf_cabinet_cbarg_t cabinet); // Add a cabinet to the currently described AS

XBT_PUBLIC(void) sg_platf_new_route (sg_platf_route_cbarg_t route); // Add a route
XBT_PUBLIC(void) sg_platf_new_ASroute (sg_platf_route_cbarg_t ASroute); // Add an ASroute
XBT_PUBLIC(void) sg_platf_new_bypassRoute (sg_platf_route_cbarg_t bypassroute); // Add a bypassRoute
XBT_PUBLIC(void) sg_platf_new_bypassASroute (sg_platf_route_cbarg_t bypassASroute); // Add an bypassASroute

XBT_PUBLIC(void) sg_platf_new_trace(sg_platf_trace_cbarg_t trace);
XBT_PUBLIC(void) sg_platf_trace_connect(sg_platf_trace_connect_cbarg_t trace_connect);

XBT_PUBLIC(void) sg_platf_new_storage(sg_platf_storage_cbarg_t storage); // Add a storage to the currently described AS
XBT_PUBLIC(void) sg_platf_new_mstorage(sg_platf_mstorage_cbarg_t mstorage);
XBT_PUBLIC(void) sg_platf_new_storage_type(sg_platf_storage_type_cbarg_t storage_type);
XBT_PUBLIC(void) sg_platf_new_mount(sg_platf_mount_cbarg_t mount);

XBT_PUBLIC(void) sg_platf_new_process(sg_platf_process_cbarg_t process);

// Add route and Asroute without xml file with those functions
XBT_PUBLIC(void) sg_platf_route_begin (sg_platf_route_cbarg_t route); // Initialize route
XBT_PUBLIC(void) sg_platf_route_end (sg_platf_route_cbarg_t route); // Finalize and add a route

XBT_PUBLIC(void) sg_platf_ASroute_begin (sg_platf_route_cbarg_t ASroute); // Initialize ASroute
XBT_PUBLIC(void) sg_platf_ASroute_end (sg_platf_route_cbarg_t ASroute); // Finalize and add a ASroute

XBT_PUBLIC(void) sg_platf_route_add_link (const char* link_id, sg_platf_route_cbarg_t route); // Add a link to link list
XBT_PUBLIC(void) sg_platf_ASroute_add_link (const char* link_id, sg_platf_route_cbarg_t ASroute); // Add a link to link list

SG_END_DECL()

#endif                          /* SG_PLATF_H */
