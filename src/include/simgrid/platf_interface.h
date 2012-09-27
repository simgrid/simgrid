/* platf_interface.h - Internal interface to the SimGrid platforms          */

/* Copyright (c) 2004, 2005, 2006, 2007, 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SG_PLATF_INTERFACE_H
#define SG_PLATF_INTERFACE_H

#include "simgrid/platf.h" /* public interface */
#include "xbt/RngStream.h"

/* Module management functions */
void sg_platf_init(void);
void sg_platf_exit(void);

/* Managing the parsing callbacks */

typedef void (*sg_platf_host_cb_t)(sg_platf_host_cbarg_t);
typedef void (*sg_platf_host_link_cb_t)(sg_platf_host_link_cbarg_t);
typedef void (*sg_platf_router_cb_t)(sg_platf_router_cbarg_t);
typedef void (*sg_platf_link_cb_t)(sg_platf_link_cbarg_t);
typedef void (*sg_platf_peer_cb_t)(sg_platf_peer_cbarg_t);
typedef void (*sg_platf_cluster_cb_t)(sg_platf_cluster_cbarg_t);
typedef void (*sg_platf_cabinet_cb_t)(sg_platf_cabinet_cbarg_t);
typedef void (*sg_platf_AS_cb_t)(sg_platf_AS_cbarg_t);
typedef void (*sg_platf_prop_cb_t)(sg_platf_prop_cbarg_t);

typedef void (*sg_platf_route_cb_t)(sg_platf_route_cbarg_t);

typedef void (*sg_platf_trace_cb_t)(sg_platf_trace_cbarg_t);
typedef void (*sg_platf_trace_connect_cb_t)(sg_platf_trace_connect_cbarg_t);

typedef void (*sg_platf_storage_cb_t)(sg_platf_storage_cbarg_t);
typedef void (*sg_platf_storage_type_cb_t)(sg_platf_storage_type_cbarg_t);
typedef void (*sg_platf_mount_cb_t)(sg_platf_mount_cbarg_t);
typedef void (*sg_platf_mstorage_cb_t)(sg_platf_mstorage_cbarg_t);

void sg_platf_host_add_cb(sg_platf_host_cb_t);
void sg_platf_host_link_add_cb(sg_platf_host_link_cb_t);
void sg_platf_router_add_cb(sg_platf_router_cb_t);
void sg_platf_link_add_cb(sg_platf_link_cb_t);
void sg_platf_peer_add_cb(sg_platf_peer_cb_t fct);
void sg_platf_cluster_add_cb(sg_platf_cluster_cb_t fct);
void sg_platf_cabinet_add_cb(sg_platf_cabinet_cb_t fct);
void sg_platf_postparse_add_cb(void_f_void_t fct);
void sg_platf_AS_begin_add_cb(sg_platf_AS_cb_t fct);
void sg_platf_AS_end_add_cb(sg_platf_AS_cb_t fct);
void sg_platf_prop_add_cb(sg_platf_prop_cb_t fct);

void sg_platf_route_add_cb(sg_platf_route_cb_t);
void sg_platf_ASroute_add_cb(sg_platf_route_cb_t);
void sg_platf_bypassRoute_add_cb(sg_platf_route_cb_t);
void sg_platf_bypassASroute_add_cb(sg_platf_route_cb_t);

void sg_platf_trace_add_cb(sg_platf_trace_cb_t);
void sg_platf_trace_connect_add_cb(sg_platf_trace_connect_cb_t);

void sg_platf_storage_add_cb(sg_platf_storage_cb_t fct);
void sg_platf_mstorage_add_cb(sg_platf_mstorage_cb_t fct);
void sg_platf_storage_type_add_cb(sg_platf_storage_type_cb_t fct);
void sg_platf_mount_add_cb(sg_platf_mount_cb_t fct);

/** \brief Pick the right models for CPU, net and workstation, and call their model_init_preparse
 *
 * Must be called within parsing/creating the environment (after the <config>s, if any, and before <AS> or friends such as <cluster>)
 */
void surf_config_models_setup(void);

/* RngStream management functions */
void sg_platf_rng_stream_init(unsigned long seed[6]);
RngStream sg_platf_rng_stream_get(const char* id);

#endif                          /* SG_PLATF_INTERFACE_H */
