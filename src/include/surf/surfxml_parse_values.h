/* Copyright (c) 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURFXML_PARSE_VALUES_H_
#define SURFXML_PARSE_VALUES_H_

typedef struct s_peer *peer_t;
typedef struct s_peer {
	char* V_peer_id;
	char* V_peer_power;
	char* V_peer_bw_in;
	char* V_peer_bw_out;
	char* V_peer_lat;
	char* V_peer_coord;
	char* V_peer_availability_trace;
	char* V_peer_state_trace;
} s_peer_t;

typedef struct s_link *link_t;
typedef struct s_link {
	char* V_link_id;
	double V_link_bandwidth;
	tmgr_trace_t V_link_bandwidth_file;
	double V_link_latency;
	tmgr_trace_t V_link_latency_file;
	e_surf_resource_state_t V_link_state;
	tmgr_trace_t V_link_state_file;
	int V_link_sharing_policy;
	int V_policy_initial_link;
} s_link_t;

typedef struct s_cluster *cluster_t;
typedef struct s_cluster {
	char* V_cluster_id;
	char* V_cluster_prefix;
	char* V_cluster_suffix;
	char* V_cluster_radical;
	double S_cluster_power;
	int S_cluster_core;
	double S_cluster_bw;
	double S_cluster_lat;
	double S_cluster_bb_bw;
	double S_cluster_bb_lat;
	char* S_cluster_router_id;
	int V_cluster_sharing_policy;
	int V_cluster_bb_sharing_policy;
	char* V_cluster_availability_file;
	char* V_cluster_state_file;
} s_cluster_t;

typedef struct s_router *router_t;
typedef struct s_router {
	char* V_router_id;
	char* V_router_coord;
} s_router_t;

typedef struct s_hostSG *hostSG_t;
typedef struct s_hostSG {
	char* V_host_id;													//id
	double V_host_power_peak;											//power
	int V_host_core;													//core
	double V_host_power_scale;											//availability
	tmgr_trace_t V_host_power_trace;									//availability file
	e_surf_resource_state_t V_host_state_initial;						//state
	tmgr_trace_t V_host_state_trace;									//state file
	char* V_host_coord;
} s_hostSG_t;

extern hostSG_t struct_host;
extern router_t struct_router;
extern cluster_t struct_cluster;
extern peer_t struct_peer;
extern link_t struct_lnk;

#endif /* SURFXML_PARSE_VALUES_H_ */
